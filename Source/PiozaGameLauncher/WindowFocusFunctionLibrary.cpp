// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "WindowFocusFunctionLibrary.h"
#include "Framework/Application/SlateApplication.h"

bool UWindowFocusFunctionLibrary::IsGameWindowFocused() {
  if (FSlateApplication::IsInitialized()) {
    return FSlateApplication::Get().IsActive();
  }

  return false;
}

UWindowFocusSubsystem *UWindowFocusFunctionLibrary::GetWindowFocusSubsystem(
    const UObject *WorldContextObject) {
  if (!WorldContextObject) {
    return nullptr;
  }

  if (UWorld *World = WorldContextObject->GetWorld()) {
    if (UGameInstance *GameInstance = World->GetGameInstance()) {
      return GameInstance->GetSubsystem<UWindowFocusSubsystem>();
    }
  }

  return nullptr;
}
