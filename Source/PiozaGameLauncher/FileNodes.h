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
 * copying directories, deleting, and browsing.
 *
 * SAFETY NOTE: Destructive operations (DeleteFile, DeleteDirectory, CopyDirectory,
 * and SaveText with bForceOverwrite) are checked against a built-in list of critical
 * system/user directories (see IsProtectedPath in the .cpp) before anything happens
 * on disk. This check is intentionally independent from any path validation you do
 * yourself elsewhere, so it still catches mistakes (bad concatenated paths, symlinks
 * pointing somewhere unexpected, etc.) even if your own logic doesn't.
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
     *
     * BREAKING CHANGE: this node gained new pins (bFollowSymlinks, bOverwriteExisting,
     * OutError). Existing Blueprint graphs calling it will need to be reconnected.
     *
     * DestDir is checked against the same protected-path safety net as DeleteDirectory,
     * and the call is refused if DestDir is the same as, or nested inside, SourceDir.
     *
     * @param SourceDir - The directory to copy from.
     * @param DestDir - The directory to copy to.
     * @param bFollowSymlinks - If false (default/recommended), symlinked files/directories found inside
     *        SourceDir are skipped rather than copied through - only "real" files and directories are
     *        copied. If true, symlink targets are copied as if they were regular content (be aware this
     *        can follow a link outside of SourceDir, and a symlink cycle could loop - a depth limit guards
     *        against infinite recursion, but the operation will simply fail once it's hit).
     * @param bOverwriteExisting - If true, files that already exist at the destination are overwritten.
     * @param OutError - Output string for an error message if the copy fails or is blocked.
     * @return true if the copy completed successfully; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool CopyDirectory(const FString& SourceDir, const FString& DestDir, bool bFollowSymlinks, bool bOverwriteExisting, FString& OutError);

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
     * @param bForceOverwrite - If true, allows overwriting read-only files. This is checked
     *        against the protected-path safety net, same as a delete would be.
     * @param Encoding - Text encoding format to use.
     * @param OutError - Output string for error message if saving fails.
     * @return true if the file was saved successfully; false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool SaveText(const FString& FilePath, const FString& Text, bool bAppend, bool bForceOverwrite, ETextEncodingFormat Encoding, FString& OutError);

    /**
     * Deletes a single file.
     * Deleting a file only ever removes that one directory entry/link - on every
     * supported platform this can never "follow" a symlink into deleting its target,
     * so there's no bFollowSymlinks parameter here (unlike DeleteDirectory).
     * @param FilePath - Path to the file to delete.
     * @param OutError - Output string for an error message if deletion fails or is blocked.
     * @return true if the file was deleted (or already didn't exist); false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool DeleteFile(const FString& FilePath, FString& OutError);

    /**
     * Deletes a directory, optionally recursively.
     *
     * Before touching anything, DirectoryPath is checked against a built-in list of
     * critical system/user locations (drive roots, /home, /etc, C:/Users, the current
     * user's actual home directory, etc.) and refused if it matches - this runs
     * independently of any validation you do yourself, as a last line of defense.
     *
     * @param DirectoryPath - Path to the directory to delete.
     * @param bRecursive - If true, deletes all contents first; if false, only succeeds on an empty directory.
     * @param bFollowSymlinks - If false (default/recommended), symlinked subdirectories encountered while
     *        recursing are left alone: only the link itself is removed, its target is never touched or
     *        recursed into. If true, the operation follows symlinks and deletes their targets too, which
     *        can escape the directory tree you intended to delete - only enable this if you fully trust
     *        every symlink that might exist inside DirectoryPath.
     * @param OutError - Output string for an error message if deletion fails or is blocked.
     * @return true if the directory was deleted (or already didn't exist); false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "FileNodes")
    static bool DeleteDirectory(const FString& DirectoryPath, bool bRecursive, bool bFollowSymlinks, FString& OutError);

    /**
     * Lists files and/or directories inside a specified directory, optionally recursively.
     * @param DirPath - Directory path to list contents from.
     * @param Pattern - Wildcard pattern to filter filenames (e.g. "*.txt").
     * @param bShowFiles - Include files in the results.
     * @param bShowDirectories - Include directories in the results.
     * @param bRecursive - If true, list contents recursively. Note: recursive listing may follow
     *        symlinked directories (this is read-only, so it's not covered by the symlink safety
     *        net used by the delete/copy operations above).
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

private:

    /**
     * Returns true if Path points at a symlink (a reparse point on Windows), false
     * otherwise - including if Path simply doesn't exist.
     */
    static bool IsSymlinkPath(const FString& Path);

    /**
     * Checks InPath (after resolving it to an absolute path, and resolving through it if
     * it is itself a symlink) against a built-in safety net of critical system/user
     * directories. This is deliberately conservative and independent of caller-side
     * validation.
     * @param OutReason - Set to a human-readable reason the path was flagged, if this returns true.
     * @return true if the path must never be deleted/overwritten by this library.
     */
    static bool IsProtectedPath(const FString& InPath, FString& OutReason);

    /** Resolves Path to an absolute, normalized directory path (no trailing slash). */
    static FString ResolveFullDirectoryPath(const FString& Path);

    /** Resolves Path to an absolute, normalized file path. */
    static FString ResolveFullFilePath(const FString& Path);

    /** Removes a directory entry that is itself a symlink, without touching its target. */
    static bool DeleteDirectoryLinkOnly(const FString& FullPath, FString& OutError);

    /** Recursive worker for DeleteDirectory. FullPath must already be resolved/validated. */
    static bool DeleteDirectoryInternal(const FString& FullPath, bool bFollowSymlinks, FString& OutError, int32 Depth = 0);

    /** Recursive worker for CopyDirectory. SourcePath/DestPath must already be resolved/validated. */
    static bool CopyDirectoryInternal(const FString& SourcePath, const FString& DestPath, bool bFollowSymlinks, bool bOverwriteExisting, FString& OutError, int32 Depth);
};
