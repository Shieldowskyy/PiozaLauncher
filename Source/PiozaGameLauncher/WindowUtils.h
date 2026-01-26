// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

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

    UFUNCTION(BlueprintCallable, Category = "Window")
    static void MinimizeToTray(const FString& Tooltip, const FString& IconPath = "");

    UFUNCTION(BlueprintCallable, Category = "Window")
    static void RestoreFromTray();
};