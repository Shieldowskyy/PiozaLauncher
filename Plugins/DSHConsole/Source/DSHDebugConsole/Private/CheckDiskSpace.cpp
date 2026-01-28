#include "CheckDiskSpace.h"
#include "Misc/Paths.h"

int64 UCheckDiskSpace::GetFreeDiskSpaceInBytes(const FString& DrivePath)
{
    if (!FPaths::DirectoryExists(DrivePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid drive path: %s"), *DrivePath);
        return -1; // Indicates an error
    }

    uint64 TotalNumberOfFreeBytes = 0;
    uint64 TotalNumberOfBytes = 0;

    if (FPlatformMisc::GetDiskTotalAndFreeSpace(DrivePath, TotalNumberOfFreeBytes, TotalNumberOfBytes))
    {
        // Returning the free space in bytes
        return TotalNumberOfFreeBytes; // This is correct
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get disk space for: %s"), *DrivePath);
        return -1; // Indicates an error
    }
}

int64 UCheckDiskSpace::GetFreeDiskSpaceInMB(const FString& DrivePath)
{
    if (!FPaths::DirectoryExists(DrivePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid drive path: %s"), *DrivePath);
        return -1; // Indicates an error
    }

    uint64 NumberOfFreeBytes = 0;
    uint64 TotalNumberOfBytes = 0;

    if (FPlatformMisc::GetDiskTotalAndFreeSpace(DrivePath, TotalNumberOfBytes, NumberOfFreeBytes))
    {
        // Convert to megabytes (1 MB = 1048576 bytes)
        return NumberOfFreeBytes / 1048576; // Correct conversion
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get disk space for: %s"), *DrivePath);
        return -1; // Indicates an error
    }
}
