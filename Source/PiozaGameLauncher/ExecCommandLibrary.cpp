// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "ExecCommandLibrary.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include <signal.h>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX
#include <dirent.h>
#include <unistd.h>
#endif

TMap<int32, FProcHandle> UExecCommandLibrary::ActiveProcesses;

static FString EscapeAndQuoteArgument(const FString& Arg)
{
    if (Arg.IsEmpty())
        return TEXT("\"\"");

    // Check if quoting is needed
    bool bNeedsQuotes = false;
    for (TCHAR C : Arg)
    {
        if (FChar::IsWhitespace(C) || C == '"' || C == '\\')
        {
            bNeedsQuotes = true;
            break;
        }
    }

    if (!bNeedsQuotes)
        return Arg;

    // Escape and quote
    FString Escaped = TEXT("\"");
    for (TCHAR C : Arg)
    {
        if (C == '"')
            Escaped += TEXT("\\\"");
        else if (C == '\\')
            Escaped += TEXT("\\\\");
        else
            Escaped += C;
    }
    Escaped += TEXT("\"");
    return Escaped;
}

static FString BuildArgumentString(const TArray<FString>& Args)
{
    TArray<FString> EscapedArgs;
    for (const FString& Arg : Args)
    {
        EscapedArgs.Add(EscapeAndQuoteArgument(Arg));
    }
    return FString::Join(EscapedArgs, TEXT(" "));
}

FString UExecCommandLibrary::ExecuteSystemCommand(
    const FString& Command,
    const TArray<FString>& Arguments,
    bool bDetached,
    bool bHidden,
    int32 Priority,
    const FString& OptionalWorkingDirectory,
    bool& bSuccess,
    int32& ProcessID)
{
    FString Output;
    ProcessID = -1;
    uint32 RealProcessID = 0;

    // Create pipes for output capture
    void* ReadPipe = nullptr;
    void* WritePipe = nullptr;
    if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
    {
        bSuccess = false;
        return TEXT("Failed to create pipes.");
    }

    // Prepare working directory
    FString WorkingDirStr = OptionalWorkingDirectory;
    if (!WorkingDirStr.IsEmpty())
    {
        WorkingDirStr = FPaths::ConvertRelativePathToFull(WorkingDirStr);
    }
    const TCHAR* WorkingDir = WorkingDirStr.IsEmpty() ? nullptr : *WorkingDirStr;

    FString ArgsString = BuildArgumentString(Arguments);
    UE_LOG(LogTemp, Log, TEXT("Executing: %s %s"), *Command, *ArgsString);

    // Start the process
    FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
        *Command,
        *ArgsString,
        bDetached,
        bHidden,
        false,
        &RealProcessID,
        Priority,
        WorkingDir,
        WritePipe,
        nullptr
    );

    if (!ProcessHandle.IsValid())
    {
        bSuccess = false;
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
        UE_LOG(LogTemp, Error, TEXT("Failed to start process: %s %s"), *Command, *ArgsString);
        return TEXT("Failed to start process.");
    }

    bSuccess = true;
    ProcessID = static_cast<int32>(RealProcessID);

    if (bDetached)
    {
        ActiveProcesses.Add(ProcessID, ProcessHandle);
        UE_LOG(LogTemp, Log, TEXT("Process started detached. PID: %d"), ProcessID);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Process started. PID: %d. Capturing output..."), ProcessID);

        // Capture output while process runs
        while (FPlatformProcess::IsProcRunning(ProcessHandle))
        {
            FString Chunk = FPlatformProcess::ReadPipe(ReadPipe);
            if (!Chunk.IsEmpty())
            {
                Output += Chunk;
            }
            FPlatformProcess::Sleep(0.01f);
        }

        // Get any remaining output
        FString FinalChunk = FPlatformProcess::ReadPipe(ReadPipe);
        if (!FinalChunk.IsEmpty())
        {
            Output += FinalChunk;
        }

        FPlatformProcess::CloseProc(ProcessHandle);
        UE_LOG(LogTemp, Log, TEXT("Process finished. PID: %d"), ProcessID);
    }

    FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
    return Output;
}

FString UExecCommandLibrary::ExecuteShellCommand(
    const FString& ShellCommandLine,
    bool bDetached,
    bool bHidden,
    int32 Priority,
    const FString& OptionalWorkingDirectory,
    bool& bSuccess,
    int32& ProcessID)
{
    FString ShellBinary;
    FString ShellArg;

    #if PLATFORM_WINDOWS
        ShellBinary = TEXT("cmd.exe");
        if (!OptionalWorkingDirectory.IsEmpty())
        {
            FString Dir = FPaths::ConvertRelativePathToFull(OptionalWorkingDirectory);
            ShellArg = FString::Printf(TEXT("/C \"cd /d \"%s\" && %s\""), *Dir, *ShellCommandLine);
        }
        else
        {
            ShellArg = TEXT("/C \"") + ShellCommandLine + TEXT("\"");
        }
    #else
        ShellBinary = TEXT("/bin/sh");
        if (!OptionalWorkingDirectory.IsEmpty())
        {
            FString Dir = FPaths::ConvertRelativePathToFull(OptionalWorkingDirectory);
            ShellArg = FString::Printf(TEXT("-c \"cd '%s' && %s\""), *Dir, *ShellCommandLine);
        }
        else
        {
            ShellArg = TEXT("-c \"") + ShellCommandLine + TEXT("\"");
        }
    #endif

    return ExecuteSystemCommand(ShellBinary, { ShellArg }, bDetached, bHidden, Priority, OptionalWorkingDirectory, bSuccess, ProcessID);
}

bool UExecCommandLibrary::IsProcessStillRunning(int32 ProcessID)
{
    if (FProcHandle* HandlePtr = ActiveProcesses.Find(ProcessID))
    {
        if (HandlePtr->IsValid())
        {
            return FPlatformProcess::IsProcRunning(*HandlePtr);
        }
    }
    return false;
}

bool UExecCommandLibrary::TerminateProcess(int32 ProcessID)
{
    if (FProcHandle* HandlePtr = ActiveProcesses.Find(ProcessID))
    {
        if (HandlePtr->IsValid())
        {
            FPlatformProcess::TerminateProc(*HandlePtr);
            FPlatformProcess::CloseProc(*HandlePtr);
            ActiveProcesses.Remove(ProcessID);
            return true;
        }
    }
    return false;
}

bool UExecCommandLibrary::TerminateProcessByName(const FString& NameFragment)
{
    bool bAnyTerminated = false;

    #if PLATFORM_WINDOWS
        HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (Snapshot == INVALID_HANDLE_VALUE)
            return false;

        PROCESSENTRY32 Entry;
        Entry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(Snapshot, &Entry))
        {
            do
            {
                FString ProcessName = Entry.szExeFile;
                if (ProcessName.Contains(NameFragment))
                {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, false, Entry.th32ProcessID);
                    if (hProcess)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Terminating process: %s (PID: %d)"), *ProcessName, Entry.th32ProcessID);
                        ::TerminateProcess(hProcess, 1);
                        CloseHandle(hProcess);
                        bAnyTerminated = true;
                    }
                }
            } while (Process32Next(Snapshot, &Entry));
        }

        CloseHandle(Snapshot);

    #elif PLATFORM_LINUX || PLATFORM_MAC
        FILE* Pipe = popen("ps -eo pid,comm", "r");
        if (!Pipe)
            return false;

        char Buffer[512];
        while (fgets(Buffer, sizeof(Buffer), Pipe))
        {
            FString Line(Buffer);
            Line = Line.TrimStartAndEnd();

            TArray<FString> Parts;
            Line.ParseIntoArrayWS(Parts);

            if (Parts.Num() >= 2)
            {
                int32 PID = FCString::Atoi(*Parts[0]);
                FString ProcName = Parts[1];

                if (ProcName.Contains(NameFragment))
                {
                    if (kill(PID, SIGTERM) == 0)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Terminated process: %s (PID: %d)"), *ProcName, PID);
                        bAnyTerminated = true;
                    }
                }
            }
        }

        pclose(Pipe);
    #endif

    return bAnyTerminated;
}

bool UExecCommandLibrary::TerminateProcessesByPathFragment(const FString& PathFragment)
{
    bool bAnyTerminated = false;

    #if PLATFORM_WINDOWS
        HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (Snapshot == INVALID_HANDLE_VALUE)
            return false;

        PROCESSENTRY32 Entry;
        Entry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(Snapshot, &Entry))
        {
            do
            {
                FString ProcPath;
                if (GetProcessExecutablePath(Entry.th32ProcessID, ProcPath))
                {
                    if (ProcPath.Contains(PathFragment))
                    {
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, false, Entry.th32ProcessID);
                        if (hProcess)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Terminating process: %s (PID: %d)"), *ProcPath, Entry.th32ProcessID);
                            ::TerminateProcess(hProcess, 1);
                            CloseHandle(hProcess);
                            bAnyTerminated = true;
                        }
                    }
                }
            } while (Process32Next(Snapshot, &Entry));
        }

        CloseHandle(Snapshot);

    #elif PLATFORM_LINUX
        DIR* Dir = opendir("/proc");
        if (!Dir)
            return false;

        struct dirent* DirEntry;

        // Helper lambda to check if string is numeric
        auto IsNumeric = [](const char* Str) -> bool
        {
            if (!Str || *Str == '\0')
                return false;
            for (const char* p = Str; *p; ++p)
            {
                if (!(*p >= '0' && *p <= '9'))
                    return false;
            }
            return true;
        };

        while ((DirEntry = readdir(Dir)) != nullptr)
        {
            if (!IsNumeric(DirEntry->d_name))
                continue;

            int PID = atoi(DirEntry->d_name);
            FString ProcPath;
            if (GetProcessExecutablePath(PID, ProcPath))
            {
                if (ProcPath.Contains(PathFragment))
                {
                    if (kill(PID, SIGTERM) == 0)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Terminated process: %s (PID: %d)"), *ProcPath, PID);
                        bAnyTerminated = true;
                    }
                }
            }
        }
        closedir(Dir);
    #endif

    return bAnyTerminated;
}

#if PLATFORM_WINDOWS
bool UExecCommandLibrary::GetProcessExecutablePath(uint32 PID, FString& OutPath)
{
    OutPath.Empty();

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, false, PID);
    if (!hProcess)
        return false;

    TCHAR Buffer[MAX_PATH] = { 0 };
    DWORD Size = MAX_PATH;

    if (GetModuleFileNameEx(hProcess, nullptr, Buffer, Size))
    {
        OutPath = FString(Buffer);
        CloseHandle(hProcess);
        return true;
    }

    CloseHandle(hProcess);
    return false;
}
#elif PLATFORM_LINUX
bool UExecCommandLibrary::GetProcessExecutablePath(int32 PID, FString& OutPath)
{
    OutPath.Empty();

    FString LinkPath = FString::Printf(TEXT("/proc/%d/exe"), PID);
    TCHAR Buffer[PATH_MAX] = { 0 };

    int Count = readlink(TCHAR_TO_ANSI(*LinkPath), (char*)Buffer, PATH_MAX - 1);
    if (Count > 0)
    {
        Buffer[Count] = '\0';
        OutPath = FString(Buffer);
        return true;
    }

    return false;
}
#endif