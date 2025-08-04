#include "FileNodes.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/Paths.h"

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
	FString FinalPattern = FPaths::Combine(DirPath, Pattern.IsEmpty() ? TEXT("*") : Pattern);

	TArray<FString> Results;
	FileManager.FindFilesRecursive(Results, *DirPath, *Pattern, bRecursive, bShowFiles, bShowDirectories);

	if (Results.Num() == 0)
		return false;

	OutNodes = Results;
	return true;
}
