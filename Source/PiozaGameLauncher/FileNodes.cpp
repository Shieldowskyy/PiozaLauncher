// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "FileNodes.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceDebug.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "GenericPlatform/GenericPlatformFile.h"

bool UFileNodes::ReadText(const FString& FilePath, FString& OutText)
{
    // Avoid FFileHelper errors in the log if the file doesn't exist
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        return false;
    }
    return FFileHelper::LoadFileToString(OutText, *FilePath);
}

bool UFileNodes::ReadBytes(const FString& FilePath, TArray<uint8>& OutBytes)
{
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        return false;
    }
    return FFileHelper::LoadFileToArray(OutBytes, *FilePath);
}

bool UFileNodes::CopyDirectory(const FString& SourceDir, const FString& DestDir)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.CopyDirectoryTree(*DestDir, *SourceDir, /* bOverwriteExisting = */ true);
}

int64 UFileNodes::GetFileSize(const FString& FilePath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.FileSize(*FilePath);
}

bool UFileNodes::SaveText(const FString& FilePath, const FString& Text, bool bAppend, bool bForceOverwrite, ETextEncodingFormat Encoding, FString& OutError)
{
    IFileManager& FileManager = IFileManager::Get();
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    // Handle read-only files if overwrite is forced
    if (bForceOverwrite && !bAppend)
    {
        if (PlatformFile.FileExists(*FilePath))
        {
            if (PlatformFile.IsReadOnly(*FilePath))
            {
                if (!PlatformFile.SetReadOnly(*FilePath, false))
                {
                    OutError = FString::Printf(TEXT("Failed to remove ReadOnly flag: %s"), *FilePath);
                    return false;
                }
            }
        }
    }

    const uint32 WriteFlags = (bAppend ? FILEWRITE_Append : FILEWRITE_None);

    // Special handling for UTF-8 without BOM (FFileHelper defaults to BOM for ForceUTF8)
    if (Encoding == ETextEncodingFormat::UTF8WithoutBOM)
    {
        FTCHARToUTF8 UTF8Converter(*Text);
        TArray<uint8> UTF8Data;
        UTF8Data.Append(reinterpret_cast<const uint8*>(UTF8Converter.Get()), UTF8Converter.Length());

        if (!FFileHelper::SaveArrayToFile(UTF8Data, *FilePath, &FileManager, WriteFlags))
        {
            OutError = FString::Printf(TEXT("Failed to write UTF-8 (no BOM) file: %s"), *FilePath);
            return false;
        }
        return true;
    }

    FFileHelper::EEncodingOptions ChosenEncoding;
    switch (Encoding)
    {
        case ETextEncodingFormat::ANSI:
            ChosenEncoding = FFileHelper::EEncodingOptions::ForceAnsi;
            break;
        case ETextEncodingFormat::UTF8:
            ChosenEncoding = FFileHelper::EEncodingOptions::ForceUTF8;
            break;
        case ETextEncodingFormat::UTF16:
            ChosenEncoding = FFileHelper::EEncodingOptions::ForceUnicode;
            break;
        case ETextEncodingFormat::AutoDetect:
        default:
            ChosenEncoding = FFileHelper::EEncodingOptions::AutoDetect;
            break;
    }

    if (!FFileHelper::SaveStringToFile(Text, *FilePath, ChosenEncoding, &FileManager, WriteFlags))
    {
        OutError = FString::Printf(TEXT("Failed to write to file: %s"), *FilePath);
        return false;
    }

    return true;
}

bool UFileNodes::ListDirectory(const FString& DirPath, const FString& Pattern, bool bShowFiles, bool bShowDirectories, bool bRecursive, TArray<FString>& OutNodes)
{
    IFileManager& FileManager = IFileManager::Get();
    const FString FinalPattern = Pattern.IsEmpty() ? TEXT("*") : Pattern;
    TArray<FString> Results;

    if (bRecursive)
    {
        FileManager.FindFilesRecursive(Results, *DirPath, *FinalPattern, bShowFiles, bShowDirectories);
    }
    else
    {
        if (bShowFiles)
        {
            FileManager.FindFiles(Results, *DirPath, *FinalPattern);
        }

        if (bShowDirectories)
        {
            TArray<FString> DirResults;
            // FindFiles works differently for directories, strictly requiring the wildcard
            FileManager.FindFiles(DirResults, *(DirPath / TEXT("*")), /* Files = */ false, /* Directories = */ true);
            Results.Append(DirResults);
        }
    }

    if (Results.Num() == 0)
    {
        return false;
    }

    OutNodes = Results;
    return true;
}

bool UFileNodes::BrowseDirectory(const FString& DirectoryPath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    FString NormalizedPath = FPaths::ConvertRelativePathToFull(DirectoryPath);
    FPaths::NormalizeDirectoryName(NormalizedPath);

    if (!PlatformFile.DirectoryExists(*NormalizedPath))
    {
        UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Path is not a valid directory: %s"), *NormalizedPath);
        return false;
    }

    #if PLATFORM_ANDROID
    UE_LOG(LogTemp, Warning, TEXT("BrowseDirectory: Not supported on Android"));
    return false;

    #elif PLATFORM_WINDOWS
    // Windows Explorer handles backslashes better in some edge cases
    NormalizedPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::IgnoreCase);

    const FString Command = TEXT("explorer.exe");
    // Quotes are required to handle paths with spaces
    const FString Params = FString::Printf(TEXT("\"%s\""), *NormalizedPath);

    FPlatformProcess::CreateProc(*Command, *Params, true, false, false, nullptr, 0, nullptr, nullptr);
    return true;

    #elif PLATFORM_LINUX
    const FString Command = TEXT("xdg-open");
    const FString Params = FString::Printf(TEXT("\"%s\""), *NormalizedPath);

    FPlatformProcess::CreateProc(*Command, *Params, true, false, false, nullptr, 0, nullptr, nullptr);
    return true;

    #else
    UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Unsupported platform"));
    return false;
    #endif
}
