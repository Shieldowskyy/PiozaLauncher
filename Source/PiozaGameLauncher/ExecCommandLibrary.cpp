// ExecCommandLibrary.cpp
#include "ExecCommandLibrary.h"
#include "HAL/PlatformProcess.h"

TMap<int32, FProcHandle> UExecCommandLibrary::ActiveProcesses;

static FString EscapeAndQuoteArgument(const FString& Arg)
{
    if (Arg.IsEmpty())
        return TEXT("\"\"");

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
    int32& ProcessID
)
{
    FString Output;
    ProcessID = -1;
    uint32 RealProcessID = 0;

    // Create pipes for output
    void* ReadPipe = nullptr;
    void* WritePipe = nullptr;
    if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
    {
        bSuccess = false;
        return TEXT("Failed to create pipes.");
    }

    const TCHAR* WorkingDir = OptionalWorkingDirectory.IsEmpty() ? nullptr : *OptionalWorkingDirectory;
    FString ArgsString = BuildArgumentString(Arguments);

    UE_LOG(LogTemp, Log, TEXT("Executing: %s %s"), *Command, *ArgsString);

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

        while (FPlatformProcess::IsProcRunning(ProcessHandle))
        {
            FString Chunk = FPlatformProcess::ReadPipe(ReadPipe);
            if (!Chunk.IsEmpty())
            {
                UE_LOG(LogTemp, Log, TEXT("%s"), *Chunk);
                Output += Chunk;
            }
            FPlatformProcess::Sleep(0.01f);
        }

        // Final read after process finishes
        FString FinalChunk = FPlatformProcess::ReadPipe(ReadPipe);
        if (!FinalChunk.IsEmpty())
        {
            UE_LOG(LogTemp, Log, TEXT("%s"), *FinalChunk);
            Output += FinalChunk;
        }

        FPlatformProcess::CloseProc(ProcessHandle);
        UE_LOG(LogTemp, Log, TEXT("Process finished. PID: %d"), ProcessID);
    }

    FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
    return Output;
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
