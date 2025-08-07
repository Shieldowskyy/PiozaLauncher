#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ExecCommandLibrary.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UExecCommandLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// BlueprintCallable, bo chcesz wywoływać z BP
	UFUNCTION(BlueprintCallable, Category = "System")
	static bool TerminateProcessByName(const FString& NameFragment);

	UFUNCTION(BlueprintCallable, Category = "System")
	static bool TerminateProcessesByPathFragment(const FString& PathFragment);

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

	UFUNCTION(BlueprintCallable, Category = "System")
	static bool IsProcessStillRunning(int32 ProcessID);

	UFUNCTION(BlueprintCallable, Category = "System")
	static bool TerminateProcess(int32 ProcessID);

private:
	static TMap<int32, FProcHandle> ActiveProcesses;

	#if PLATFORM_WINDOWS
	static bool GetProcessExecutablePath(uint32 PID, FString& OutPath);
	#elif PLATFORM_LINUX
	static bool GetProcessExecutablePath(int32 PID, FString& OutPath);
	#endif
};
