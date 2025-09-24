// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System;

public class PiozaGameLauncher : ModuleRules
{
    public PiozaGameLauncher(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Slate",
            "SlateCore",
            "UMG",
            "ApplicationCore",
            "Json",
            "JsonUtilities",
            "StructUtils"
        });

        // Uncomment if you are using Slate UI
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
        
        // Add build date definition
        PublicDefinitions.Add("BUILD_DATE=\"" + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + "\"");
    }
}
