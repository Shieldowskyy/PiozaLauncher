// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "SystemTraySubsystem.h"

// =====================================================================
// TEMPORARILY DISABLED - skipped on all platforms until further notice
// =====================================================================

void UTrayMenuItem::SetLabel(const FString& InLabel) {}
void UTrayMenuItem::SetEnabled(bool bInEnabled) {}
void UTrayMenuItem::RemoveFromTray() {}
void UTrayMenuItem::NotifyParentRefresh() {}

void USystemTraySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void USystemTraySubsystem::Deinitialize()
{
    Super::Deinitialize();
}

FString USystemTraySubsystem::GetBestIconPath() const
{
    return FString();
}

bool USystemTraySubsystem::IsApplicationInTray() const
{
    return false;
}

void USystemTraySubsystem::ShowTrayIcon(const FString& Tooltip, const FString& IconPath) {}
void USystemTraySubsystem::HideTrayIcon() {}

UTrayMenuItem* USystemTraySubsystem::CreateTrayMenuItem(const FString& Label, bool bIsEnabled, bool bIsSeparator)
{
    return nullptr;
}

void USystemTraySubsystem::AddTrayMenuItem(UTrayMenuItem* MenuItem) {}
void USystemTraySubsystem::InsertTrayMenuItem(UTrayMenuItem* MenuItem, int32 Index) {}
void USystemTraySubsystem::RemoveTrayMenuItem(UTrayMenuItem* MenuItem) {}
void USystemTraySubsystem::ClearTrayMenuItems(bool bKeepTitle) {}
void USystemTraySubsystem::RefreshTrayMenu() {}

void USystemTraySubsystem::OnRequestDestroyWindowOverride(const TSharedRef<SWindow>& InWindow) {}

bool USystemTraySubsystem::TickSubsystem(float DeltaTime)
{
    return true;
}
