#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ExecCommandLibrary.generated.h"

/**
 * Blueprint library for executing and managing system processes.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UExecCommandLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Terminates all processes containing the specified name fragment.
     * @param NameFragment - Part of the process name to match.
     * @return true if at least one process was terminated successfully.
     */
    UFUNCTION(BlueprintCallable, Category = "System")
    static bool TerminateProcessByName(const FString& NameFragment);

    /**
     * Terminates all processes whose executable path contains the specified fragment.
     * @param PathFragment - Part of the executable path to match.
     * @return true if at least one process was terminated successfully.
     */
    UFUNCTION(BlueprintCallable, Category = "System")
    static bool TerminateProcessesByPathFragment(const FString& PathFragment);

    /**
     * Executes a system command with given arguments.
     * @param Command - The executable or command to run.
     * @param Arguments - Array of arguments for the command.
     * @param bDetached - If true, process runs detached from the parent.
     * @param bHidden - If true, process window will be hidden.
     * @param Priority - Process priority class or nice value.
     * @param OptionalWorkingDirectory - Optional working directory for the process.
     * @param bSuccess - Output, true if process started successfully.
     * @param ProcessID - Output, the process ID of the started process.
     * @return Standard output from the executed process.
     */
    UFUNCTION(BlueprintCallable, Category = "System")
    static FString ExecuteSystemCommand(
        const FString& Command,
        const TArray<FString>& Arguments,
        bool bDetached,
        bool bHidden,
        int32 Priority,
        const FString& OptionalWorkingDirectory,
        bool& bSuccess,
        int32& ProcessID
    );

    /**
     * Executes a shell command line.
     * @param ShellCommandLine - Full shell command line to execute.
     * @param bDetached - If true, process runs detached.
     * @param bHidden - If true, process window will be hidden.
     * @param Priority - Process priority.
     * @param OptionalWorkingDirectory - Optional working directory.
     * @param bSuccess - Output, true if process started successfully.
     * @param ProcessID - Output, ID of the started process.
     * @return Standard output from the executed shell command.
     */
    UFUNCTION(BlueprintCallable, Category = "System")
    static FString ExecuteShellCommand(
        const FString& ShellCommandLine,
        bool bDetached,
        bool bHidden,
        int32 Priority,
        const FString& OptionalWorkingDirectory,
        bool& bSuccess,
        int32& ProcessID
    );

    /**
     * Checks whether a process with given ProcessID is still running.
     * @param ProcessID - ID of the process to check.
     * @return true if process is running; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "System")
    static bool IsProcessStillRunning(int32 ProcessID);

    /**
     * Terminates the process with given ProcessID.
     * @param ProcessID - ID of the process to terminate.
     * @return true if the process was terminated successfully.
     */
    UFUNCTION(BlueprintCallable, Category = "System")
    static bool TerminateProcess(int32 ProcessID);

private:
    /** Map of active processes tracked by their ProcessID */
    static TMap<int32, FProcHandle> ActiveProcesses;

#if PLATFORM_WINDOWS
    /** Retrieves the executable path for a process by PID (Windows) */
    static bool GetProcessExecutablePath(uint32 PID, FString& OutPath);
#elif PLATFORM_LINUX
    /** Retrieves the executable path for a process by PID (Linux) */
    static bool GetProcessExecutablePath(int32 PID, FString& OutPath);
#endif
};
