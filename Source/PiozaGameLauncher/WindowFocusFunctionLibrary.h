// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "WindowFocusSubsystem.h"
#include "WindowFocusFunctionLibrary.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UWindowFocusFunctionLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /** Checks if game window has focus. */
  UFUNCTION(BlueprintPure, Category = "Window")
  static bool IsGameWindowFocused();

  /** Gets WindowFocusSubsystem for binding to focus events. */
  UFUNCTION(BlueprintPure, Category = "Window",
            meta = (WorldContext = "WorldContextObject"))
  static UWindowFocusSubsystem *
  GetWindowFocusSubsystem(const UObject *WorldContextObject);
};