// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IconUtilsLibrary.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UIconUtilsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Resolves the full path to an icon based on its name and system theme.
	 */
	UFUNCTION(BlueprintCallable, Category = "Icon Utils")
	static FString ResolveIconPath(const FString& IconName, int32 PreferredSize = 256);

	/**
	 * Returns default system paths where icons are located.
	 */
	UFUNCTION(BlueprintCallable, Category = "Icon Utils")
	static TArray<FString> GetIconSearchPaths();

	/**
	 * Loads an icon (from file or executable) as a Texture2D.
	 */
	UFUNCTION(BlueprintCallable, Category = "Icon Utils")
	static class UTexture2D* LoadIconAsTexture(const FString& IconPath, int32 IconIndex = 0);

	/**
	 * Saves a Texture2D to a file (PNG, JPEG, BMP).
	 */
	UFUNCTION(BlueprintCallable, Category = "Icon Utils")
	static bool SaveTextureToFile(class UTexture2D* Texture, const FString& FilePath);

#if PLATFORM_WINDOWS
	static class UTexture2D* CreateTextureFromHIcon(void* InHIcon);
	static void* GetWindowsIconHandle(const FString& FilePath, int32 IconIndex);
#endif
};
