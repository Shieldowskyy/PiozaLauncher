#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileNodes.generated.h"

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
	static bool SaveText(const FString& FilePath, const FString& Text, bool bAppend, bool bForceOverwrite, FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "FileNodes")
	static bool ListDirectory(const FString& DirPath, const FString& Pattern, bool bShowFiles, bool bShowDirectories, bool bRecursive, TArray<FString>& OutNodes);

	UFUNCTION(BlueprintCallable, Category = "FileNodes")
	static bool BrowseDirectory(const FString& DirectoryPath);
};
