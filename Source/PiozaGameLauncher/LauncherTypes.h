// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "LauncherTypes.generated.h" 

USTRUCT(BlueprintType)
struct FAllowedFeatures
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bMove = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bBackup = true;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bRestore = true;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bSelectRHI = true;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bCustomGames = true;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bThemes = true;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bDebugConsole = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
	bool bVerifyAndRepair = true;
	
};
