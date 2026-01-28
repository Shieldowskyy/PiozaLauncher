// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ChecksumLibrary.h" // Needed for EChecksumAlgorithm
#include "ChecksumLibraryAsync.generated.h"

/**
 * Struct to hold the results of a bulk verification process.
 */
USTRUCT(BlueprintType)
struct FVerificationResult
{
	GENERATED_BODY()

	/** Files that failed checksum verification (Relative Paths) */
	UPROPERTY(BlueprintReadOnly, Category = "Checksum")
	TArray<FString> CorruptedFiles;

	/** Files that were not found on disk (Relative Paths) */
	UPROPERTY(BlueprintReadOnly, Category = "Checksum")
	TArray<FString> MissingFiles;

	/** Total number of files processed (excluding ignored ones) */
	UPROPERTY(BlueprintReadOnly, Category = "Checksum")
	int32 TotalFilesChecked = 0;
};

// Single-file callback
DECLARE_DYNAMIC_DELEGATE_TwoParams(FVerifyChecksumResult, bool, bSuccess, bool, bMatch);

/**
 * Delegate for progress updates.
 * @param ProgressPercent - Value between 0.0 and 1.0.
 * @param CurrentFile - The relative path of the file currently being processed.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnVerificationProgress, float, ProgressPercent, FString, CurrentFile);

/**
 * Delegate for completion.
 * @param Result - Structure containing lists of corrupted and missing files.
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnVerificationComplete, FVerificationResult, Result);

/**
 * Async Blueprint wrapper for checksum verification operations.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UChecksumLibraryAsync : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Verify a single file’s checksum without blocking the game thread.
	 */
	UFUNCTION(BlueprintCallable, Category = "File|Checksum", meta = (AutoCreateRefTerm = "ResultCallback"))
	static bool VerifyFileChecksumAsync(const FString& FilePath,
										const FString& ExpectedChecksum,
									 EChecksumAlgorithm Algorithm,
									 const FVerifyChecksumResult& ResultCallback);

	/**
	 * Verifies a list of files using multi-threading (ParallelFor).
	 * Optimized for high performance and UI responsiveness.
	 *
	 * @param RelativeFilePaths List of file paths relative to GameDirectory (e.g. "Binaries/Win64/Game.exe").
	 * @param ExpectedChecksums Map of RelativePath -> Checksum to compare against.
	 * @param GameDirectory     Absolute path to the game root folder.
	 * @param FilesToIgnore     List of patterns to skip (supports wildcards like "*.log" or "Config*").
	 * @param Algorithm         The hashing algorithm to use.
	 * @param OnProgress        Event fired to update UI (throttled).
	 * @param OnComplete        Event fired when verification is finished.
	 */
	UFUNCTION(BlueprintCallable, Category = "File|Checksum", meta = (AutoCreateRefTerm = "OnProgress,OnComplete,FilesToIgnore"))
	static void VerifyFileListAsync(const TArray<FString>& RelativeFilePaths,
									const TMap<FString, FString>& ExpectedChecksums,
								 const FString& GameDirectory,
								 const TArray<FString>& FilesToIgnore,
								 EChecksumAlgorithm Algorithm,
								 const FOnVerificationProgress& OnProgress,
								 const FOnVerificationComplete& OnComplete);
};
