#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ExecCommandLibrary.generated.h"

/**
 * A Blueprint function library that allows executing system commands
 * with various execution options such as detached mode, hidden window, priority, and working directory.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UExecCommandLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Executes a system command with optional arguments and execution settings.
	 * @param Command The command or program to execute.
	 * @param Arguments A list of additional arguments for the command.
	 * @param bDetached Whether the process should run detached (true) or block execution (false).
	 * @param bHidden Whether the process window should be hidden (true) or visible (false).
	 * @param Priority The priority of the process.
	 * @param OptionalWorkingDirectory The working directory for the process.
	 * @param bSuccess Returns true if the process started successfully.
	 * @param ProcessID Returns the process ID of the started process.
	 * @return The output from the command execution.
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
};