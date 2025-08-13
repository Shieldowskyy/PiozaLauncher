#include "FileNodes.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceDebug.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "GenericPlatform/GenericPlatformFile.h"

bool UFileNodes::ReadText(const FString& FilePath, FString& OutText)
{
    return FFileHelper::LoadFileToString(OutText, *FilePath);
}

bool UFileNodes::ReadBytes(const FString& FilePath, TArray<uint8>& OutBytes)
{
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
    if (PlatformFile.FileExists(*FilePath))
    {
        return PlatformFile.FileSize(*FilePath);
    }
    return -1;
}

bool UFileNodes::SaveText(const FString& FilePath, const FString& Text, bool bAppend, bool bForceOverwrite, ETextEncodingFormat Encoding, FString& OutError)
{
    const uint32 WriteFlags = (bAppend ? FILEWRITE_Append : FILEWRITE_None) | (bForceOverwrite ? FILEWRITE_AllowRead : FILEWRITE_None);

    // Special case for UTF8 without BOM
    if (Encoding == ETextEncodingFormat::UTF8WithoutBOM)
    {
        FTCHARToUTF8 UTF8Converter(*Text);
        TArray<uint8> UTF8Data;
        UTF8Data.Append(reinterpret_cast<const uint8*>(UTF8Converter.Get()), UTF8Converter.Length());

        if (!FFileHelper::SaveArrayToFile(UTF8Data, *FilePath, &IFileManager::Get(), WriteFlags))
        {
            OutError = FString::Printf(TEXT("Failed to write UTF-8 (no BOM) file: %s"), *FilePath);
            return false;
        }
        return true;
    }

    // Map ETextEncodingFormat to FFileHelper encoding options
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

    if (!FFileHelper::SaveStringToFile(Text, *FilePath, ChosenEncoding, &IFileManager::Get(), WriteFlags))
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
        FileManager.FindFilesRecursive(Results, *DirPath, *FinalPattern, /* Files = */ true, bShowFiles, bShowDirectories);
    }
    else
    {
        // FindFiles only finds files, not directories; need extra call if directories are wanted
        if (bShowFiles)
        {
            FileManager.FindFiles(Results, *DirPath, *FinalPattern);
        }

        if (bShowDirectories)
        {
            TArray<FString> DirResults;
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

    // Convert to absolute path and normalize directory name (remove trailing slash)
    FString NormalizedPath = FPaths::ConvertRelativePathToFull(DirectoryPath);
    FPaths::NormalizeDirectoryName(NormalizedPath);

    if (!PlatformFile.DirectoryExists(*NormalizedPath))
    {
        UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Path is not a valid directory: %s"), *NormalizedPath);
        return false;
    }

#if PLATFORM_WINDOWS
    // Use backslashes on Windows
    NormalizedPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::IgnoreCase);

    // Do NOT add trailing slash or quotes
    const FString Command = TEXT("explorer.exe");
    const FString Params = NormalizedPath;

#elif PLATFORM_LINUX
    const FString Command = TEXT("xdg-open");
    const FString Params = FString::Printf(TEXT("\"%s\""), *NormalizedPath);

#else
    UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Unsupported platform"));
    return false;
#endif

    FPlatformProcess::CreateProc(*Command, *Params, /* bLaunchDetached */ true, /* bLaunchHidden */ false, /* bLaunchReallyHidden */ false, nullptr, 0, nullptr, nullptr);

    return true;
}
