// Fill out your copyright notice in the Description page of Project Settings.

#include "PiozaGameLauncher.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"

class FPiozaGameLauncherModule : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();
        
        // Automatycznie dodaj flagę AllowSoftwareRendering do command line
        FString CurrentCommandLine = FCommandLine::Get();
        if (!CurrentCommandLine.Contains(TEXT("AllowSoftwareRendering")))
        {
            CurrentCommandLine += TEXT(" -AllowSoftwareRendering");
            FCommandLine::Set(*CurrentCommandLine);
            
            UE_LOG(LogTemp, Log, TEXT("Software rendering enabled automatically"));
        }
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FPiozaGameLauncherModule, PiozaGameLauncher, "PiozaGameLauncher");