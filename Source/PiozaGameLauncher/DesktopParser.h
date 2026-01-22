// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DesktopParser.generated.h"

USTRUCT(BlueprintType)
struct FDesktopEntryInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	FString Name;

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	FString ExecutablePath;

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	FString ExecutableName;

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	TArray<FString> Arguments;

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	FString WorkingDirectory;

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	FString IconPath;

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	FString Comment;

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	FString FilePath;

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	bool bIsValid = false;

	UPROPERTY(BlueprintReadWrite, Category = "Desktop Entry")
	int32 IconIndex = 0;
};

UCLASS()
class PIOZAGAMELAUNCHER_API UDesktopParser : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static TArray<FString> GetDefaultSearchPaths();

	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static TArray<FString> GetAllDesktopFiles(const TArray<FString>& SearchPaths);

	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static FDesktopEntryInfo ParseDesktopFile(const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static TArray<FDesktopEntryInfo> ParseMultipleDesktopFiles(const TArray<FString>& FilePaths);

	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static FString GetExecutableNameFromPath(const FString& ExecutablePath);

	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static FString ResolveIconPath(const FString& IconName, int32 PreferredSize = 256);

	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static TArray<FString> GetIconSearchPaths();

	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static FString ResolveExecutablePath(const FString& ExecutableName);

	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static UTexture2D* LoadIconAsTexture(const FString& IconPath, int32 IconIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Desktop Parser")
	static bool SaveTextureToFile(UTexture2D* Texture, const FString& FilePath);

private:
	static FDesktopEntryInfo ParseLinuxDesktopFile(const FString& FilePath);
	static FDesktopEntryInfo ParseWindowsShortcut(const FString& FilePath);
	static void SplitCommandLine(const FString& FullCommandLine, FString& OutExecutable, TArray<FString>& OutArguments);
	static TArray<FString> TokenizeCommandLine(const FString& CommandLine);
	
#if PLATFORM_WINDOWS
	static UTexture2D* CreateTextureFromHIcon(void* InHIcon);
	static void* GetWindowsIconHandle(const FString& FilePath, int32 IconIndex);
#endif
};