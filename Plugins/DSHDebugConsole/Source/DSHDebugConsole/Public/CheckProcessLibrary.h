#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoreMinimal.h"
#include "CheckProcessLibrary.generated.h"

UCLASS()
class DSHDEBUGCONSOLE_API UCheckProcessLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Function to check if a process is running
    UFUNCTION(BlueprintCallable, Category = "Process Check")
    static bool IsProcessRunning(const FString& ProcessName);
};
