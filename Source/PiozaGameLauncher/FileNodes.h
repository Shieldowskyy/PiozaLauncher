#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileNodes.generated.h"

UENUM(BlueprintType)
enum class ETextEncodingFormat : uint8
{
	AutoDetect       UMETA(DisplayName = "Auto Detect"),
	ANSI             UMETA(DisplayName = "ANSI"),
	UTF8             UMETA(DisplayName = "UTF-8 (with BOM)"),
	UTF8WithoutBOM   UMETA(DisplayName = "UTF-8 (no BOM)"),
	UTF16            UMETA(DisplayName = "UTF-16 LE")
};


UCLASS()
class PIOZAGAMELAUNCHER_API UFileNodes : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "FileNodes")
	static bool ReadText(const FString& FilePath, FString& OutText);

	UFUNCTION(BlueprintCallable, Category = "FileNodes")
	static bool ReadBytes(const FString& FilePath, TArray<uint8>& OutBytes);

	UFUNCTION(BlueprintCallable, Category = "FileNodes")
	static bool CopyDirectory(const FString& SourceDir, const FString& DestDir);

	UFUNCTION(BlueprintCallable, Category = "FileNodes")
	static int64 GetFileSize(const FString& FilePath);

	UFUNCTION(BlueprintCallable, Category = "FileNodes")
	static bool SaveText(const FString& FilePath, const FString& Text, bool bAppend, bool bForceOverwrite, ETextEncodingFormat Encoding, FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "FileNodes")
	static bool ListDirectory(const FString& DirPath, const FString& Pattern, bool bShowFiles, bool bShowDirectories, bool bRecursive, TArray<FString>& OutNodes);

	UFUNCTION(BlueprintCallable, Category = "FileNodes")
	static bool BrowseDirectory(const FString& DirectoryPath);
};
