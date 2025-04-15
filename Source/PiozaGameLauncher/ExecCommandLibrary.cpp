#include "ExecCommandLibrary.h"
#include "HAL/PlatformProcess.h"

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

    // Combine arguments into a single string
    FString ArgumentsString = FString::Join(Arguments, TEXT(" "));
    FString FullCommand = Command + TEXT(" ") + ArgumentsString;

#if PLATFORM_WINDOWS
    FString Executable = Command;
    FString Params = ArgumentsString;
#elif PLATFORM_LINUX
    FString Executable = TEXT("/bin/bash");
    FString Params = TEXT("-c \"") + FullCommand + TEXT("\"");
#endif

    void* ReadPipe = nullptr;
    void* WritePipe = nullptr;
    FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

    // Process creation flags
    bool bLaunchDetached = bDetached;
    bool bLaunchHidden = bHidden;
    bool bLaunchAsAdmin = false;

    // Create the process with the given settings
    FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
        *Executable,
        *Params,
        bLaunchDetached,
        bLaunchHidden,
        bLaunchAsAdmin,
        &RealProcessID,
        Priority,
        !OptionalWorkingDirectory.IsEmpty() ? *OptionalWorkingDirectory : nullptr,
        WritePipe
    );
    
    if (ProcessHandle.IsValid())
    {
        bSuccess = true;
        Output = FPlatformProcess::ReadPipe(ReadPipe);

        if (!bDetached)
        {
            FPlatformProcess::WaitForProc(ProcessHandle);
        }
        
        FPlatformProcess::CloseProc(ProcessHandle);
        ProcessID = static_cast<int32>(RealProcessID);
    }
    else
    {
        bSuccess = false;
        Output = TEXT("Failed to start process.");
        ProcessID = -1;
    }

    FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
    return Output;
}
