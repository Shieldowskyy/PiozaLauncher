#include "PiozaDiskSpace.h"
#include "Misc/Paths.h"

int64 UPiozaDiskSpace::GetFreeDiskSpaceInBytes(const FString& DrivePath)
{
    if (!FPaths::DirectoryExists(DrivePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid drive path: %s"), *DrivePath);
        return -1; // Sygnalizuje b³¹d
    }

    uint64 TotalNumberOfFreeBytes = 0;
    uint64 TotalNumberOfBytes = 0;

    if (FPlatformMisc::GetDiskTotalAndFreeSpace(DrivePath, TotalNumberOfFreeBytes, TotalNumberOfBytes))
    {
        // Konwersja na megabajty
        return TotalNumberOfFreeBytes;// / 1024; // to jest Ÿle XD
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get disk space for: %s"), *DrivePath);
        return -1; // Sygnalizuje b³¹d
    }
}

int64 UPiozaDiskSpace::GetFreeDiskSpaceInMB(const FString& DrivePath)
{
    if (!FPaths::DirectoryExists(DrivePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid drive path: %s"), *DrivePath);
        return -1; // Sygnalizuje b³¹d
    }

    uint64 NumberOfFreeBytes = 0;
    uint64 TotalNumberOfBytes = 0;

    if (FPlatformMisc::GetDiskTotalAndFreeSpace(DrivePath, TotalNumberOfBytes, NumberOfFreeBytes))
    {
        // Konwersja na megabajty
        return NumberOfFreeBytes / 1048576; // to jest Ÿle XD
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to get disk space for: %s"), *DrivePath);
        return -1; // Sygnalizuje b³¹d
    }
}