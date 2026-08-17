// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "ExecCommandLibrary.h"
#include "ProcessTrackerLibrary.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include <signal.h>
#include <stdio.h>

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <TlHelp32.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

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

// Safeguard function to prevent accidentally killing critical system or GUI processes
static bool IsSafeToTerminate(int32 PID, const FString& ProcessNameOrPath)
{
    // 1. Protect the Launcher itself from committing suicide
    if (static_cast<uint32>(PID) == FPlatformProcess::GetCurrentProcessId())
    {
        return false;
    }

    // 2. Protect critical low-PID system processes (e.g., init, systemd, kernel threads)
    if (PID <= 10)
    {
        return false;
    }

    // 3. Extended Blacklist of critical processes (Linux GUI, daemons, Windows Core)
    static const TArray<FString> ProtectedFragments = {
        TEXT("xorg"), TEXT("wayland"), TEXT("gnome-shell"), TEXT("plasmashell"),
        TEXT("kwin"), TEXT("mutter"), TEXT("xfwm4"), TEXT("lxqt"),
        TEXT("cinnamon"), TEXT("mate-panel"), TEXT("cosmic"), TEXT("sway"),
        TEXT("hyprland"), TEXT("budgie"), TEXT("pantheon"), TEXT("xfce"),
        TEXT("lxde"), TEXT("enlightenment"), TEXT("i3"), TEXT("bspwm"),
        TEXT("awesome"), TEXT("openbox"), TEXT("fluxbox"), TEXT("labwc"),
        TEXT("wlroots"), TEXT("weston"), TEXT("picom"), TEXT("compton"),
        TEXT("sddm"), TEXT("gdm"), TEXT("gdm3"), TEXT("lightdm"), TEXT("kdeinit"), TEXT("ly"), TEXT("greetd"),
        TEXT("systemd"), TEXT("init"), TEXT("dbus"), TEXT("pulseaudio"),
        TEXT("pipewire"), TEXT("wireplumber"), TEXT("sshd"), TEXT("bash"),
        TEXT("zsh"), TEXT("login"), TEXT("polkit"), TEXT("rtkit"),
        TEXT("explorer.exe"), TEXT("svchost.exe"), TEXT("csrss.exe"),
        TEXT("wininit.exe"), TEXT("smss.exe"), TEXT("lsass.exe"),
        TEXT("services.exe"), TEXT("winlogon.exe"), TEXT("dwm.exe")
    };

    FString LowerName = ProcessNameOrPath.ToLower();
    for (const FString& Protected : ProtectedFragments)
    {
        if (LowerName.Contains(Protected))
        {
            return false;
        }
    }

    return true;
}

// Builds a map of ProcessID -> ParentProcessID for the entire system
static TMap<uint32, uint32> BuildParentProcessMap()
{
    TMap<uint32, uint32> ParentMap;

    #if PLATFORM_WINDOWS
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 Entry;
        Entry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(Snapshot, &Entry))
        {
            do {
                ParentMap.Add(Entry.th32ProcessID, Entry.th32ParentProcessID);
            } while (Process32Next(Snapshot, &Entry));
        }
        CloseHandle(Snapshot);
    }
    #elif PLATFORM_LINUX || PLATFORM_MAC
    // Use ps to get pid and parent pid (ppid)
    FILE* Pipe = popen("ps -eo pid,ppid", "r");
    if (Pipe)
    {
        char Buffer[256];
        // Skip header line
        if (fgets(Buffer, sizeof(Buffer), Pipe)) {}

        while (fgets(Buffer, sizeof(Buffer), Pipe))
        {
            FString Line(Buffer);
            Line = Line.TrimStartAndEnd();
            TArray<FString> Parts;
            Line.ParseIntoArrayWS(Parts);

            if (Parts.Num() >= 2)
            {
                uint32 PID = static_cast<uint32>(FCString::Atoi(*Parts[0]));
                uint32 PPID = static_cast<uint32>(FCString::Atoi(*Parts[1]));
                ParentMap.Add(PID, PPID);
            }
        }
        pclose(Pipe);
    }
    #endif

    return ParentMap;
}

// Checks if TargetPID is a descendant of RootPID by climbing up the parent chain
static bool IsDescendantOf(uint32 TargetPID, uint32 RootPID, const TMap<uint32, uint32>& ParentMap)
{
    uint32 CurrentPID = TargetPID;

    while (CurrentPID > 0)
    {
        if (CurrentPID == RootPID)
        {
            return true;
        }

        const uint32* ParentPID = ParentMap.Find(CurrentPID);
        // Prevent infinite loops in case of malformed OS data (PID == PPID)
        if (ParentPID && *ParentPID != CurrentPID && *ParentPID != 0)
        {
            CurrentPID = *ParentPID;
        }
        else
        {
            break;
        }
    }

    return false;
}

class FScopedEnvironmentVariables
{
public:
    explicit FScopedEnvironmentVariables(const TMap<FString, FString>& VarsToSet)
    {
        PreviousValues.Reserve(VarsToSet.Num());
        for (const TPair<FString, FString>& Pair : VarsToSet)
        {
            FString PreviousValue = FPlatformMisc::GetEnvironmentVariable(*Pair.Key);
            PreviousValues.Add(Pair.Key, PreviousValue);
            FPlatformMisc::SetEnvironmentVar(*Pair.Key, *Pair.Value);
        }
    }

    ~FScopedEnvironmentVariables()
    {
        for (const TPair<FString, FString>& Pair : PreviousValues)
        {
            FPlatformMisc::SetEnvironmentVar(*Pair.Key, *Pair.Value);
        }
    }

private:
    TMap<FString, FString> PreviousValues;
};

FString UExecCommandLibrary::ExecuteSystemCommand(
    const FString& Command,
    const TArray<FString>& Arguments,
    bool bDetached,
    bool bHidden,
    int32 Priority,
    const FString& OptionalWorkingDirectory,
    const TMap<FString, FString>& EnvironmentVariables,
    bool& bSuccess,
    int32& ProcessID)
{
    FString Output;
    ProcessID = -1;
    uint32 RealProcessID = 0;

    void* ReadPipe = nullptr;
    void* WritePipe = nullptr;
    if (!bDetached)
    {
        if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
        {
            bSuccess = false;
            return TEXT("Failed to create pipes.");
        }
    }

    FString WorkingDirStr = OptionalWorkingDirectory;
    if (!WorkingDirStr.IsEmpty())
    {
        WorkingDirStr = FPaths::ConvertRelativePathToFull(WorkingDirStr);
    }
    const TCHAR* WorkingDir = WorkingDirStr.IsEmpty() ? nullptr : *WorkingDirStr;

    FString ArgsString = BuildArgumentString(Arguments);
    UE_LOG(LogTemp, Log, TEXT("Executing: %s %s"), *Command, *ArgsString);

    FProcHandle ProcessHandle;
    {
        FScopedEnvironmentVariables ScopedEnv(EnvironmentVariables);

        ProcessHandle = FPlatformProcess::CreateProc(
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
    }

    if (!ProcessHandle.IsValid())
    {
        bSuccess = false;
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
        UE_LOG(LogTemp, Error, TEXT("Failed to start process: %s %s"), *Command, *ArgsString);
        return TEXT("Failed to start process.");
    }

    bSuccess = true;
    ProcessID = static_cast<int32>(RealProcessID);
    UProcessTrackerLibrary::RegisterProcess(ProcessID, ProcessHandle);

    if (bDetached)
    {
        UE_LOG(LogTemp, Log, TEXT("Process started detached. PID: %d"), ProcessID);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Process started. PID: %d. Capturing output..."), ProcessID);

        while (FPlatformProcess::IsProcRunning(ProcessHandle))
        {
            FString Chunk = FPlatformProcess::ReadPipe(ReadPipe);
            if (!Chunk.IsEmpty())
            {
                Output += Chunk;
            }
            FPlatformProcess::Sleep(0.01f);
        }

        FString FinalChunk = FPlatformProcess::ReadPipe(ReadPipe);
        if (!FinalChunk.IsEmpty())
        {
            Output += FinalChunk;
        }

        FPlatformProcess::CloseProc(ProcessHandle);
        UE_LOG(LogTemp, Log, TEXT("Process finished. PID: %d"), ProcessID);
    }

    if (ReadPipe || WritePipe)
    {
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
    }
    return Output;
}

FString UExecCommandLibrary::ExecuteShellCommand(
    const FString& ShellCommandLine,
    bool bDetached,
    bool bHidden,
    int32 Priority,
    const FString& OptionalWorkingDirectory,
    const TMap<FString, FString>& EnvironmentVariables,
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

    return ExecuteSystemCommand(ShellBinary, { ShellArg }, bDetached, bHidden, Priority, OptionalWorkingDirectory, EnvironmentVariables, bSuccess, ProcessID);
}

bool UExecCommandLibrary::TerminateProcess(int32 ProcessID, float GracefulTimeoutSeconds)
{
    TSet<uint32> Tree;
    if (!UProcessTrackerLibrary::GetTrackedTree(ProcessID, Tree))
    {
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("TerminateProcess: sending graceful terminate to %d process(es) in tree rooted at PID %d"), Tree.Num(), ProcessID);

    // --- Phase 1: graceful terminate ---
    for (uint32 PID : Tree)
    {
        FString ProcPath;
        UProcessTrackerLibrary::GetProcessExecutablePath(PID, ProcPath);

        if (!ProcPath.IsEmpty() && !IsSafeToTerminate(PID, ProcPath))
        {
            UE_LOG(LogTemp, Warning, TEXT("TerminateProcess: Blocked attempt to gracefully terminate protected process PID %d (%s)"), PID, *ProcPath);
            continue;
        }

        FProcHandle Handle = FPlatformProcess::OpenProcess(PID);
        if (Handle.IsValid())
        {
            FPlatformProcess::TerminateProc(Handle);
            FPlatformProcess::CloseProc(Handle);
        }
    }

    // --- Phase 2: wait ---
    if (GracefulTimeoutSeconds > 0.0f)
    {
        const double DeadlineSeconds = FPlatformTime::Seconds() + GracefulTimeoutSeconds;
        const float PollIntervalSeconds = 0.1f;

        for (;;)
        {
            bool bAnyStillAlive = false;
            for (uint32 PID : Tree)
            {
                FProcHandle Handle = FPlatformProcess::OpenProcess(PID);
                if (Handle.IsValid())
                {
                    if (FPlatformProcess::IsProcRunning(Handle))
                    {
                        bAnyStillAlive = true;
                    }
                    FPlatformProcess::CloseProc(Handle);
                }
                if (bAnyStillAlive)
                {
                    break;
                }
            }

            if (!bAnyStillAlive || FPlatformTime::Seconds() >= DeadlineSeconds)
            {
                break;
            }

            FPlatformProcess::Sleep(PollIntervalSeconds);
        }
    }

    // --- Phase 3: force-kill ---
    int32 ForceKilledCount = 0;
    for (uint32 PID : Tree)
    {
        FProcHandle Handle = FPlatformProcess::OpenProcess(PID);
        if (Handle.IsValid())
        {
            if (FPlatformProcess::IsProcRunning(Handle))
            {
                FString ProcPath;
                UProcessTrackerLibrary::GetProcessExecutablePath(PID, ProcPath);

                if (!ProcPath.IsEmpty() && !IsSafeToTerminate(PID, ProcPath))
                {
                    UE_LOG(LogTemp, Warning, TEXT("TerminateProcess: Blocked attempt to force-kill protected process PID %d (%s)"), PID, *ProcPath);
                    FPlatformProcess::CloseProc(Handle);
                    continue;
                }

                ++ForceKilledCount;
                #if PLATFORM_LINUX || PLATFORM_MAC
                kill((pid_t)PID, SIGKILL);
                #else
                FPlatformProcess::TerminateProc(Handle);
                #endif
            }
            FPlatformProcess::CloseProc(Handle);
        }
    }

    if (ForceKilledCount > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("TerminateProcess: force-killed %d process(es) that did not exit gracefully (tree rooted at PID %d)"), ForceKilledCount, ProcessID);
    }

    UProcessTrackerLibrary::ClearTracking(ProcessID);
    return true;
}

bool UExecCommandLibrary::TerminateProcessByName(const FString& NameFragment, bool bOnlyChildrenOfLauncher)
{
    bool bAnyTerminated = false;
    uint32 LauncherPID = FPlatformProcess::GetCurrentProcessId();
    TMap<uint32, uint32> ParentMap;

    if (bOnlyChildrenOfLauncher)
    {
        ParentMap = BuildParentProcessMap();
    }

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
                // LINEAGE CHECK
                if (bOnlyChildrenOfLauncher && !IsDescendantOf(Entry.th32ProcessID, LauncherPID, ParentMap))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Blocked attempt to terminate process '%s' (PID: %d) - it was NOT spawned by the launcher!"), *ProcessName, Entry.th32ProcessID);
                    continue;
                }

                // BLACKLIST CHECK
                if (!IsSafeToTerminate(Entry.th32ProcessID, ProcessName))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Blocked attempt to terminate protected process: %s (PID: %d)"), *ProcessName, Entry.th32ProcessID);
                    continue;
                }

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
            uint32 PID = static_cast<uint32>(FCString::Atoi(*Parts[0]));
            FString ProcName = Parts[1];

            if (ProcName.Contains(NameFragment))
            {
                // LINEAGE CHECK
                if (bOnlyChildrenOfLauncher && !IsDescendantOf(PID, LauncherPID, ParentMap))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Blocked attempt to terminate process '%s' (PID: %d) - it was NOT spawned by the launcher!"), *ProcName, PID);
                    continue;
                }

                // BLACKLIST CHECK
                if (!IsSafeToTerminate(PID, ProcName))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Blocked attempt to terminate protected process: %s (PID: %d)"), *ProcName, PID);
                    continue;
                }

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

bool UExecCommandLibrary::TerminateProcessesByPathFragment(const FString& PathFragment, bool bOnlyChildrenOfLauncher)
{
    bool bAnyTerminated = false;
    uint32 LauncherPID = FPlatformProcess::GetCurrentProcessId();
    TMap<uint32, uint32> ParentMap;

    if (bOnlyChildrenOfLauncher)
    {
        ParentMap = BuildParentProcessMap();
    }

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
            if (UProcessTrackerLibrary::GetProcessExecutablePath(Entry.th32ProcessID, ProcPath))
            {
                if (ProcPath.Contains(PathFragment))
                {
                    // LINEAGE CHECK
                    if (bOnlyChildrenOfLauncher && !IsDescendantOf(Entry.th32ProcessID, LauncherPID, ParentMap))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Blocked attempt to terminate process by path '%s' (PID: %d) - it was NOT spawned by the launcher!"), *ProcPath, Entry.th32ProcessID);
                        continue;
                    }

                    // BLACKLIST CHECK
                    if (!IsSafeToTerminate(Entry.th32ProcessID, ProcPath))
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Blocked attempt to terminate protected process (path): %s (PID: %d)"), *ProcPath, Entry.th32ProcessID);
                        continue;
                    }

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

        uint32 PID = static_cast<uint32>(atoi(DirEntry->d_name));
        FString ProcPath;
        if (UProcessTrackerLibrary::GetProcessExecutablePath(PID, ProcPath))
        {
            if (ProcPath.Contains(PathFragment))
            {
                // LINEAGE CHECK
                if (bOnlyChildrenOfLauncher && !IsDescendantOf(PID, LauncherPID, ParentMap))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Blocked attempt to terminate process by path '%s' (PID: %d) - it was NOT spawned by the launcher!"), *ProcPath, PID);
                    continue;
                }

                // BLACKLIST CHECK
                if (!IsSafeToTerminate(PID, ProcPath))
                {
                    UE_LOG(LogTemp, Warning, TEXT("Blocked attempt to terminate protected process (path): %s (PID: %d)"), *ProcPath, PID);
                    continue;
                }

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
