// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ChecksumLibrary.generated.h"

UENUM(BlueprintType)
enum class EChecksumAlgorithm : uint8
{
    MD5      UMETA(DisplayName = "MD5 (Fastest, least secure)"),
    SHA1     UMETA(DisplayName = "SHA-1 (Fast, deprecated security)"),
    CRC32    UMETA(DisplayName = "CRC-32 (Fastest, weakest)")
};

/**
 * Blueprint Function Library for file checksum verification
 * Supports MD5, SHA1, SHA256, and CRC32 algorithms
 * Works on Windows, Linux, and Android without external dependencies
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UChecksumLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Calculate checksum of a file using specified algorithm
     * @param FilePath - Absolute path to the file
     * @param Algorithm - Checksum algorithm to use
     * @param OutChecksum - Resulting checksum as hex string (lowercase)
     * @return True if successful, false if file not found or error occurred
     */
    UFUNCTION(BlueprintCallable, Category = "File|Checksum")
    static bool CalculateFileChecksum(const FString& FilePath, EChecksumAlgorithm Algorithm, FString& OutChecksum);

    /**
     * Verify file against expected checksum
     * @param FilePath - Absolute path to the file
     * @param ExpectedChecksum - Expected checksum as hex string (case insensitive)
     * @param Algorithm - Checksum algorithm to use
     * @return True if checksum matches, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "File|Checksum")
    static bool VerifyFileChecksum(const FString& FilePath, const FString& ExpectedChecksum, EChecksumAlgorithm Algorithm);

    /**
     * Load checksums from a text file (format: "checksum filepath" per line)
     * @param ChecksumFilePath - Path to checksums.txt file
     * @param OutChecksums - Map of filepath -> checksum
     * @return True if file loaded successfully
     */
    UFUNCTION(BlueprintCallable, Category = "File|Checksum")
    static bool LoadChecksumsFromFile(const FString& ChecksumFilePath, TMap<FString, FString>& OutChecksums);

    /**
     * Get human-readable name of the algorithm
     */
    UFUNCTION(BlueprintPure, Category = "File|Checksum")
    static FString GetAlgorithmName(EChecksumAlgorithm Algorithm);

private:
    // Internal helper functions
    static bool ReadFileInChunks(const FString& FilePath, TFunction<void(const uint8*, int32)> ProcessChunk);
    static FString BytesToHexString(const uint8* Bytes, int32 Length);
};