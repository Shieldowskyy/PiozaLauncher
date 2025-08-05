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
	return PlatformFile.CopyDirectoryTree(*DestDir, *SourceDir, true);
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

bool UFileNodes::SaveText(const FString& FilePath, const FString& Text, bool bAppend, bool bForceOverwrite, FString& OutError)
{
	const uint32 WriteFlags = (bAppend ? FILEWRITE_Append : FILEWRITE_None) | (bForceOverwrite ? FILEWRITE_AllowRead : FILEWRITE_None);

	if (!FFileHelper::SaveStringToFile(Text, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), WriteFlags))
	{
		OutError = FString::Printf(TEXT("Failed to write to file: %s"), *FilePath);
		return false;
	}

	return true;
}

bool UFileNodes::ListDirectory(const FString& DirPath, const FString& Pattern, bool bShowFiles, bool bShowDirectories, bool bRecursive, TArray<FString>& OutNodes)
{
	IFileManager& FileManager = IFileManager::Get();
	FString FinalPattern = Pattern.IsEmpty() ? TEXT("*") : Pattern;

	TArray<FString> Results;
	FileManager.FindFilesRecursive(Results, *DirPath, *FinalPattern, bRecursive, bShowFiles, bShowDirectories);

	if (Results.Num() == 0)
		return false;

	OutNodes = Results;
	return true;
}

bool UFileNodes::BrowseDirectory(const FString& DirectoryPath)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// Convert to absolute and normalize
	FString NormalizedPath = FPaths::ConvertRelativePathToFull(DirectoryPath);
	FPaths::NormalizeDirectoryName(NormalizedPath); // Removed ending slashes

	// Check if the directory exists
	if (!PlatformFile.DirectoryExists(*NormalizedPath))
	{
		UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Path is not a valid directory: %s"), *NormalizedPath);
		return false;
	}

#if PLATFORM_WINDOWS
	// Replace slashes with backslashes for Windows
	NormalizedPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::IgnoreCase);

	// Don't add a trailing backslash, and don't wrap in quotes
	FString Command = TEXT("explorer.exe");
	FString Params = NormalizedPath;

#elif PLATFORM_LINUX
	FString Command = TEXT("xdg-open");
	FString Params = FString::Printf(TEXT("\"%s\""), *NormalizedPath);
#else
	UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Unsupported platform"));
	return false;
#endif

	// Launch the process
	FPlatformProcess::CreateProc(*Command, *Params, true, false, false, nullptr, 0, nullptr, nullptr);

	return true;
}


