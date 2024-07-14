#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PiozaDiskSpace.generated.h"

UCLASS()
class UPiozaDiskSpace : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Disk Space")
    static int64 GetFreeDiskSpaceInBytes(const FString& DrivePath);

    UFUNCTION(BlueprintCallable, Category = "Disk Space")
    static int64 GetFreeDiskSpaceInMB(const FString& DrivePath);
};
