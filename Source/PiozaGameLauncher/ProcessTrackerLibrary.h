// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HAL/PlatformProcess.h"
#include "ProcessTrackerLibrary.generated.h"

/**
 * Blueprint library for tracking system processes and their child trees.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UProcessTrackerLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Checks whether a process with given ProcessID is still running (including its child tree).
     * @param ProcessID - ID of the process to check.
     * @return true if process or any of its children are running; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "System|Tracking")
    static bool IsProcessStillRunning(int32 ProcessID);

    /**
     * Registers a process and its handle for tracking.
     * @param ProcessID - PID of the process.
     * @param Handle - Proc handle.
     */
    static void RegisterProcess(int32 ProcessID, FProcHandle Handle);

    /**
     * Gets the current tracked process tree for a given Root PID.
     * @param ProcessID - Root PID.
     * @param OutTree - Output set of PIDs in the tree.
     * @return true if the process is being tracked.
     */
    static bool GetTrackedTree(int32 ProcessID, TSet<uint32>& OutTree);

    /**
     * Clears tracking data for a given ProcessID.
     */
    static void ClearTracking(int32 ProcessID);

#if PLATFORM_WINDOWS
    /** Retrieves the executable path for a process by PID (Windows) */
    static bool GetProcessExecutablePath(uint32 PID, FString& OutPath);
#elif PLATFORM_LINUX
    /** Retrieves the executable path for a process by PID (Linux) */
    static bool GetProcessExecutablePath(int32 PID, FString& OutPath);
#endif

private:
    /** Map of active processes tracked by their ProcessID */
    static TMap<int32, FProcHandle> ActiveProcesses;

    /** Map of process trees (Root PID -> Set of Child PIDs) */
    static TMap<int32, TSet<uint32>> TrackedProcessTrees;

    /** Updates the process tree for the given Root PID by scanning system processes */
    static void UpdateProcessTree(int32 RootPID);

    /** Cached parent-to-children relationship map for the entire system */
    static TMap<uint32, TArray<uint32>> CachedParentMap;
    
    /** Timestamp of the last system-wide process scan */
    static double LastScanTime;

};
