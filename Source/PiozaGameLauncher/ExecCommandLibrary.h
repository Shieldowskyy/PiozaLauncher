// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

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
     * @param EnvironmentVariables - Optional additional environment variables (Name -> Value) to set for the child process, on top of the inherited parent environment.
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
        const TMap<FString, FString>& EnvironmentVariables,
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
     * @param EnvironmentVariables - Optional additional environment variables (Name -> Value) to set for the child process, on top of the inherited parent environment.
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
        const TMap<FString, FString>& EnvironmentVariables,
        bool& bSuccess,
        int32& ProcessID
    );


    /**
     * Terminates the process with given ProcessID, along with its entire tracked child tree
     * (e.g. all processes spawned by a launcher script like umu-run: python3, wineserver,
     * winetricks, pressure-vessel, pv-adverb, etc).
     *
     * Sends a graceful termination signal (SIGTERM on Linux/Mac, TerminateProc on Windows) to
     * every process in the tree first. If GracefulTimeoutSeconds elapses and some processes in
     * the tree are still alive, those remaining processes are force-killed (SIGKILL on Linux/Mac).
     *
     * @param ProcessID - Root PID of the process tree to terminate.
     * @param GracefulTimeoutSeconds - How long to wait after the graceful signal before force-killing survivors. Use 0 to force-kill immediately without waiting.
     * @return true if the process tree was found and a termination attempt was made.
     */
    UFUNCTION(BlueprintCallable, Category = "System")
    static bool TerminateProcess(int32 ProcessID, float GracefulTimeoutSeconds = 3.0f);

};
