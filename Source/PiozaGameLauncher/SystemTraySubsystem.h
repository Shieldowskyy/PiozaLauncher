// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "SystemTraySubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnTrayIconClickedNative);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrayIconClicked);

/**
 * Subsystem for managing the system tray icon on Windows and Linux.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API USystemTraySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    void ShowTrayIcon(const FString& Tooltip, const FString& IconPath = "");

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    void HideTrayIcon();

    UPROPERTY(BlueprintAssignable, Category = "System Tray")
    FOnTrayIconClicked OnTrayIconClicked;

    FOnTrayIconClickedNative OnTrayIconClickedNative;

    FString LastTooltip;
    FString LastIconPath;

private:
    void OnRequestDestroyWindowOverride(const TSharedRef<SWindow>& InWindow);

private:
#if PLATFORM_WINDOWS
    void* TrayIconData = nullptr;
    static USystemTraySubsystem* Instance;
    bool bIsIconVisible = false;

    // Windows Message Handler
    bool HandleWindowsMessage(void* hWnd, uint32 Message, uintptr_t WParam, intptr_t LParam, intptr_t* OutResult);
#endif

#if PLATFORM_LINUX
    void* NativeDBusConnection = nullptr;
    bool bIsIconVisible = false;
    
    // Linux specific methods
    void InitLinuxDBus();
    void ShutdownLinuxDBus();
#endif
    bool TickSubsystem(float DeltaTime);
    FTSTicker::FDelegateHandle TickerHandle;
};
