// ExecCommandLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ExecCommandLibrary.generated.h"

/**
 * Blueprint function library for executing external system commands with
 * process tracking and live output capturing.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UExecCommandLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Execute a system command with a list of arguments.
	 * @param Command - Full path to the executable or script.
	 * @param Arguments - Array of arguments passed separately (will be properly escaped).
	 * @param bDetached - Whether to launch the process detached (non-blocking).
	 * @param bHidden - Whether to hide the process window (Windows only).
	 * @param Priority - Process priority.
	 * @param OptionalWorkingDirectory - Working directory for the process (optional).
	 * @param bSuccess - Output parameter indicating if the process was started successfully.
	 * @param ProcessID - Output process ID of the launched process.
	 * @return Captured standard output and error combined (empty if detached).
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
	 * Checks if a process with the given ProcessID is still running.
	 */
	UFUNCTION(BlueprintCallable, Category = "System")
	static bool IsProcessStillRunning(int32 ProcessID);

	/**
	 * Terminates a running process by ProcessID.
	 */
	UFUNCTION(BlueprintCallable, Category = "System")
	static bool TerminateProcess(int32 ProcessID);

private:
	// Map of active processes for tracking detached processes
	static TMap<int32, FProcHandle> ActiveProcesses;
};
