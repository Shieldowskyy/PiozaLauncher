// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

using UnrealBuildTool;
using System.Collections.Generic;

public class PiozaGameLauncherTarget : TargetRules
{
	public PiozaGameLauncherTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;

		ExtraModuleNames.AddRange( new string[] { "PiozaGameLauncher" } );
	}
}
