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

    // Helper lambda to escape arguments
    auto EscapeArgument = [](const FString& Arg) -> FString {
        #if PLATFORM_WINDOWS
        return TEXT("\"") + Arg.ReplaceCharWithEscapedChar() + TEXT("\"");
        #else
        FString Escaped = Arg;
        Escaped.ReplaceInline(TEXT("\\"), TEXT("\\\\")); // Backslashes
        Escaped.ReplaceInline(TEXT("\""), TEXT("\\\"")); // Double quotes
        Escaped.ReplaceInline(TEXT("$"), TEXT("\\$"));   // Dollar signs
        Escaped.ReplaceInline(TEXT("`"), TEXT("\\`"));   // Backticks
        Escaped.ReplaceInline(TEXT("!"), TEXT("\\!"));   // Bangs
        return TEXT("\"") + Escaped + TEXT("\"");
        #endif
    };

    // Escape each argument
    TArray<FString> EscapedArguments;
    for (const FString& Arg : Arguments)
    {
        EscapedArguments.Add(EscapeArgument(Arg));
    }

    FString ArgumentsString = FString::Join(EscapedArguments, TEXT(" "));
    FString FullCommand = Command + TEXT(" ") + ArgumentsString;

    #if PLATFORM_WINDOWS
    FString Executable = Command;
    FString Params = ArgumentsString;
    #elif PLATFORM_LINUX
    FString Executable = Command;
    FString Params = ArgumentsString;
    #endif

    void* ReadPipe = nullptr;
    void* WritePipe = nullptr;
    FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

    bool bLaunchDetached = bDetached;
    bool bLaunchHidden = bHidden;
    bool bLaunchAsAdmin = false;

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
