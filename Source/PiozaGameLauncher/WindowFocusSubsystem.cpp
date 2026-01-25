// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "WindowFocusSubsystem.h"
#include "Framework/Application/SlateApplication.h"

void UWindowFocusSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);

  if (FSlateApplication::IsInitialized()) {
    ActivationDelegateHandle =
        FSlateApplication::Get()
            .OnApplicationActivationStateChanged()
            .AddUObject(this,
                        &UWindowFocusSubsystem::OnApplicationActivationChanged);
  }
}

void UWindowFocusSubsystem::Deinitialize() {
  if (FSlateApplication::IsInitialized() &&
      ActivationDelegateHandle.IsValid()) {
    FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(
        ActivationDelegateHandle);
    ActivationDelegateHandle.Reset();
  }

  Super::Deinitialize();
}

void UWindowFocusSubsystem::OnApplicationActivationChanged(bool bIsActive) {
  if (bIsActive) {
    OnWindowGainFocus.Broadcast();
  } else {
    OnWindowLostFocus.Broadcast();
  }
}
