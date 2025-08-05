#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "WindowUtils.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UWindowUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Window")
    static void MinimizeWindow();

    UFUNCTION(BlueprintCallable, Category = "Window")
    static void RestoreWindow();
};