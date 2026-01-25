// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WindowFocusSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWindowGainFocus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWindowLostFocus);

UCLASS()
class PIOZAGAMELAUNCHER_API UWindowFocusSubsystem
    : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  virtual void Initialize(FSubsystemCollectionBase &Collection) override;
  virtual void Deinitialize() override;

  UPROPERTY(BlueprintAssignable, Category = "Window Focus")
  FOnWindowGainFocus OnWindowGainFocus;

  UPROPERTY(BlueprintAssignable, Category = "Window Focus")
  FOnWindowLostFocus OnWindowLostFocus;

private:
  void OnApplicationActivationChanged(bool bIsActive);
  FDelegateHandle ActivationDelegateHandle;
};
