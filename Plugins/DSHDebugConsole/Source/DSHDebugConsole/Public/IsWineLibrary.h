#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "IsWineLibrary.generated.h"

UCLASS()
class DSHDEBUGCONSOLE_API UIsWineLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Checks if the game is running under Wine/Proton on a Windows system */
    UFUNCTION(BlueprintPure, Category = "System")
    static bool IsRunningUnderWine();
};
