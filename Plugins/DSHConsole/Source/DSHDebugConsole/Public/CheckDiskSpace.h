#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CheckDiskSpace.generated.h"

UCLASS()
class DSHDEBUGCONSOLE_API UCheckDiskSpace : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Disk Space")
    static int64 GetFreeDiskSpaceInBytes(const FString& DrivePath);

    UFUNCTION(BlueprintCallable, Category = "Disk Space")
    static int64 GetFreeDiskSpaceInMB(const FString& DrivePath);
};
