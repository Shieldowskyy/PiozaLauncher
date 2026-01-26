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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnApplicationMinimizedToTray);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnApplicationRestoredFromTray);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrayMenuItemClicked, class UTrayMenuItem*, MenuItem);

UCLASS(BlueprintType)
class PIOZAGAMELAUNCHER_API UTrayMenuItem : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "System Tray")
    int32 InternalId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Tray")
    FString Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Tray")
    bool bIsEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Tray")
    bool bIsSeparator = false;

    UPROPERTY(BlueprintAssignable, Category = "System Tray")
    FOnTrayMenuItemClicked OnClicked;

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    void SetLabel(const FString& InLabel);

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    void SetEnabled(bool bInEnabled);

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    void RemoveFromTray();

private:
    void NotifyParentRefresh();
};

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

    UPROPERTY(BlueprintAssignable, Category = "System Tray")
    FOnApplicationMinimizedToTray OnApplicationMinimizedToTray;

    UPROPERTY(BlueprintAssignable, Category = "System Tray")
    FOnApplicationRestoredFromTray OnApplicationRestoredFromTray;

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    UTrayMenuItem* CreateTrayMenuItem(const FString& Label, bool bIsEnabled = true, bool bIsSeparator = false);

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    void AddTrayMenuItem(UTrayMenuItem* MenuItem);

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    void InsertTrayMenuItem(UTrayMenuItem* MenuItem, int32 Index);

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    void RemoveTrayMenuItem(UTrayMenuItem* MenuItem);

    UFUNCTION(BlueprintCallable, Category = "System Tray")
    void ClearTrayMenuItems(bool bKeepTitle = true);

    UFUNCTION(BlueprintPure, Category = "System Tray")
    const TArray<UTrayMenuItem*>& GetTrayMenuItems() const { return MenuItems; }

    UFUNCTION(BlueprintPure, Category = "System Tray")
    bool IsApplicationInTray() const;

    FOnTrayIconClickedNative OnTrayIconClickedNative;

    FString LastTooltip;
    FString LastIconPath;

    UPROPERTY()
    TArray<UTrayMenuItem*> MenuItems;

    void RefreshTrayMenu();
    int32 NextInternalId = 1;

private:
    bool bIsIconVisible = false;

    void OnRequestDestroyWindowOverride(const TSharedRef<SWindow>& InWindow);
    FString GetBestIconPath() const;
    bool bHasShownInitialTrayIcon = false;

private:
#if PLATFORM_WINDOWS
    void* TrayIconData = nullptr;
    static USystemTraySubsystem* Instance;

    // Windows Message Handler
    bool HandleWindowsMessage(void* hWnd, uint32 Message, uintptr_t WParam, intptr_t LParam, intptr_t* OutResult);
    void ShowWindowsContextMenu(void* hWnd);
#endif

#if PLATFORM_LINUX
    void* NativeDBusConnection = nullptr;
    
    // Linux specific methods
    void InitLinuxDBus();
    void ShutdownLinuxDBus();
#endif
    bool TickSubsystem(float DeltaTime);
    FTSTicker::FDelegateHandle TickerHandle;
};
