// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileNodes.generated.h"

/**
 * Enum describing supported text encoding formats for file operations.
 */
UENUM(BlueprintType)
enum class ETextEncodingFormat : uint8
{
    AutoDetect       UMETA(DisplayName = "Auto Detect"),
    ANSI             UMETA(DisplayName = "ANSI"),
    UTF8             UMETA(DisplayName = "UTF-8 (with BOM)"),
    UTF8WithoutBOM   UMETA(DisplayName = "UTF-8 (no BOM)"),
    UTF16            UMETA(DisplayName = "UTF-16 LE")
};


/**
 * Blueprint library providing common file operations like reading, writing,
 * copying directories, and browsing.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UFileNodes : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Reads the entire text content from a file.
     * @param FilePath - Path to the file to read.
     * @param OutText - Output string to receive the file content.
     * @return true if the file was successfully read; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool ReadText(const FString& FilePath, FString& OutText);

    /**
     * Reads the entire binary content from a file.
     * @param FilePath - Path to the file to read.
     * @param OutBytes - Output array to receive the file bytes.
     * @return true if the file was successfully read; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool ReadBytes(const FString& FilePath, TArray<uint8>& OutBytes);

    /**
     * Copies a directory tree recursively from source to destination.
     * @param SourceDir - The directory to copy from.
     * @param DestDir - The directory to copy to.
     * @return true if the copy was successful; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool CopyDirectory(const FString& SourceDir, const FString& DestDir);

    /**
     * Returns the size of a file in bytes.
     * @param FilePath - Path to the file.
     * @return File size in bytes, or -1 if the file does not exist.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static int64 GetFileSize(const FString& FilePath);

    /**
     * Saves text content to a file with specified encoding and write options.
     * @param FilePath - Path to the file to write.
     * @param Text - Text content to save.
     * @param bAppend - If true, appends to the existing file; otherwise overwrites.
     * @param bForceOverwrite - If true, allows overwriting read-only files.
     * @param Encoding - Text encoding format to use.
     * @param OutError - Output string for error message if saving fails.
     * @return true if the file was saved successfully; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool SaveText(const FString& FilePath, const FString& Text, bool bAppend, bool bForceOverwrite, ETextEncodingFormat Encoding, FString& OutError);

    /**
     * Lists files and/or directories inside a specified directory, optionally recursively.
     * @param DirPath - Directory path to list contents from.
     * @param Pattern - Wildcard pattern to filter filenames (e.g. "*.txt").
     * @param bShowFiles - Include files in the results.
     * @param bShowDirectories - Include directories in the results.
     * @param bRecursive - If true, list contents recursively.
     * @param OutNodes - Output array of file/directory paths found.
     * @return true if any items were found; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool ListDirectory(const FString& DirPath, const FString& Pattern, bool bShowFiles, bool bShowDirectories, bool bRecursive, TArray<FString>& OutNodes);

    /**
     * Opens the given directory in the platform's file explorer.
     * @param DirectoryPath - Path to the directory to open.
     * @return true if the directory was successfully opened; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool BrowseDirectory(const FString& DirectoryPath);
};
