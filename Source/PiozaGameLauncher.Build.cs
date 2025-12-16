// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

using UnrealBuildTool;
using System;
using System.IO;

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

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.Add("Launch");
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "AndroidFileProviderFix_APL.xml"));
		}
        
		// Add build date definition
		PublicDefinitions.Add("BUILD_DATE=\"" + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + "\"");
	}
}
