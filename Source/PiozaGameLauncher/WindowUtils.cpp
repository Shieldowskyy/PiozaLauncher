// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "WindowUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "SystemTraySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

void UWindowUtils::MinimizeWindow()
{
    TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow();
    if (Window.IsValid())
    {
        Window->Minimize();
    }
}

void UWindowUtils::RestoreWindow()
{
    TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow();
    if (Window.IsValid())
    {
        Window->Restore();
    }
}

void UWindowUtils::MinimizeToTray(const FString& Tooltip, const FString& IconPath)
{
    TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow();
    if (Window.IsValid())
    {
        Window->HideWindow();
    }

    if (GEngine && GEngine->GetWorldContexts().Num() > 0)
    {
        UGameInstance* GI = GEngine->GetWorldContexts()[0].OwningGameInstance;
        if (GI)
        {
            USystemTraySubsystem* TraySubsystem = GI->GetSubsystem<USystemTraySubsystem>();
            if (TraySubsystem)
            {
                TraySubsystem->ShowTrayIcon(Tooltip, IconPath);
                // Hook to restore window when icon is clicked
                if (!TraySubsystem->OnTrayIconClickedNative.IsBound())
                {
                    TraySubsystem->OnTrayIconClickedNative.AddStatic(&UWindowUtils::RestoreFromTray);
                }
            }
        }
    }
}

void UWindowUtils::RestoreFromTray()
{
    TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow();
    if (Window.IsValid())
    {
        Window->ShowWindow();
        Window->BringToFront();
    }

    if (GEngine && GEngine->GetWorldContexts().Num() > 0)
    {
        UGameInstance* GI = GEngine->GetWorldContexts()[0].OwningGameInstance;
        if (GI)
        {
            USystemTraySubsystem* TraySubsystem = GI->GetSubsystem<USystemTraySubsystem>();
            if (TraySubsystem)
            {
                TraySubsystem->HideTrayIcon();
            }
        }
    }
}
