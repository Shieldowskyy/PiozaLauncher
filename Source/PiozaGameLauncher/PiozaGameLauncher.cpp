// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details
#include "PiozaGameLauncher.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"

class FPiozaGameLauncherModule : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();
        
#if !WITH_EDITOR
        FString CurrentCommandLine = FCommandLine::Get();
        bool bModified = false;
        
        if (!CurrentCommandLine.Contains(TEXT("AllowSoftwareRendering")))
        {
            CurrentCommandLine += TEXT(" -AllowSoftwareRendering");
            bModified = true;
        }
        
        if (!CurrentCommandLine.Contains(TEXT("FeatureLevelES31")))
        {
            CurrentCommandLine += TEXT(" -FeatureLevelES31");
            bModified = true;
        }
        
        if (bModified)
        {
            FCommandLine::Set(*CurrentCommandLine);
            UE_LOG(LogTemp, Log, TEXT("Command line updated: %s"), *CurrentCommandLine);
        }
#endif
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FPiozaGameLauncherModule, PiozaGameLauncher, "PiozaGameLauncher");