// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "DesktopParser.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

TArray<FString> UDesktopParser::GetDefaultSearchPaths()
{
	TArray<FString> Paths;

#if PLATFORM_LINUX
	FString HomeDir = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
	
	Paths.Add(HomeDir + TEXT("/.local/share/applications"));
	Paths.Add(TEXT("/usr/share/applications"));
	Paths.Add(TEXT("/usr/local/share/applications"));
	Paths.Add(TEXT("/var/lib/flatpak/exports/share/applications"));
	Paths.Add(HomeDir + TEXT("/.local/share/flatpak/exports/share/applications"));
	Paths.Add(TEXT("/var/lib/snapd/desktop/applications"));
	
#elif PLATFORM_WINDOWS
	TCHAR AppDataPath[MAX_PATH];
	TCHAR ProgramDataPath[MAX_PATH];
	
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, AppDataPath)))
	{
		Paths.Add(FString(AppDataPath) + TEXT("\\Microsoft\\Windows\\Start Menu\\Programs"));
	}
	
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, ProgramDataPath)))
	{
		Paths.Add(FString(ProgramDataPath) + TEXT("\\Microsoft\\Windows\\Start Menu\\Programs"));
	}
	
	TCHAR StartMenuPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_STARTMENU, NULL, 0, StartMenuPath)))
	{
		Paths.Add(FString(StartMenuPath) + TEXT("\\Programs"));
	}
#endif

	return Paths;
}

TArray<FString> UDesktopParser::GetAllDesktopFiles(const TArray<FString>& SearchPaths)
{
	TArray<FString> FoundFiles;
	IFileManager& FileManager = IFileManager::Get();

#if PLATFORM_LINUX
	const FString Extension = TEXT("*.desktop");
#elif PLATFORM_WINDOWS
	const FString Extension = TEXT("*.lnk");
#else
	return FoundFiles;
#endif

	for (const FString& Path : SearchPaths)
	{
		if (!FileManager.DirectoryExists(*Path))
		{
			continue;
		}

		TArray<FString> FilesInPath;
		FileManager.FindFilesRecursive(FilesInPath, *Path, *Extension, true, false);
		FoundFiles.Append(FilesInPath);
	}

	return FoundFiles;
}

FDesktopEntryInfo UDesktopParser::ParseDesktopFile(const FString& FilePath)
{
	FDesktopEntryInfo Entry;
	Entry.FilePath = FilePath;

	if (!FPaths::FileExists(FilePath))
	{
		return Entry;
	}

#if PLATFORM_LINUX
	return ParseLinuxDesktopFile(FilePath);
#elif PLATFORM_WINDOWS
	return ParseWindowsShortcut(FilePath);
#else
	return Entry;
#endif
}

TArray<FDesktopEntryInfo> UDesktopParser::ParseMultipleDesktopFiles(const TArray<FString>& FilePaths)
{
	TArray<FDesktopEntryInfo> Entries;
	
	for (const FString& FilePath : FilePaths)
	{
		FDesktopEntryInfo Entry = ParseDesktopFile(FilePath);
		if (Entry.bIsValid)
		{
			Entries.Add(Entry);
		}
	}

	return Entries;
}

FString UDesktopParser::GetExecutableNameFromPath(const FString& ExecutablePath)
{
	FString FileName = FPaths::GetCleanFilename(ExecutablePath);
	return FileName;
}

TArray<FString> UDesktopParser::GetIconSearchPaths()
{
	TArray<FString> Paths;

#if PLATFORM_LINUX
	FString HomeDir = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
	
	Paths.Add(HomeDir + TEXT("/.local/share/icons"));
	Paths.Add(HomeDir + TEXT("/.icons"));
	Paths.Add(TEXT("/usr/share/icons"));
	Paths.Add(TEXT("/usr/share/pixmaps"));
	Paths.Add(TEXT("/var/lib/flatpak/exports/share/icons"));
	Paths.Add(HomeDir + TEXT("/.local/share/flatpak/exports/share/icons"));
	
#elif PLATFORM_WINDOWS
	TCHAR SystemPath[MAX_PATH];
	if (GetSystemDirectory(SystemPath, MAX_PATH))
	{
		Paths.Add(FString(SystemPath) + TEXT("\\..\\..\\Program Files"));
	}
#endif

	return Paths;
}

FString UDesktopParser::ResolveIconPath(const FString& IconName, int32 PreferredSize)
{
	if (IconName.IsEmpty())
	{
		return FString();
	}

	if (FPaths::FileExists(IconName))
	{
		return IconName;
	}

#if PLATFORM_LINUX
	TArray<FString> IconExtensions = { TEXT(".png"), TEXT(".svg"), TEXT(".xpm"), TEXT(".jpg") };
	TArray<FString> SearchPaths = GetIconSearchPaths();
	TArray<FString> CommonThemes = { TEXT("hicolor"), TEXT("Adwaita"), TEXT("breeze"), TEXT("default") };
	TArray<FString> SizeVariants = { 
		FString::Printf(TEXT("%dx%d"), PreferredSize, PreferredSize),
		TEXT("256x256"), TEXT("scalable"), TEXT("512x512"), TEXT("128x128"), 
		TEXT("64x64"), TEXT("48x48"), TEXT("32x32")
	};

	for (const FString& BasePath : SearchPaths)
	{
		for (const FString& Theme : CommonThemes)
		{
			for (const FString& Size : SizeVariants)
			{
				for (const FString& Ext : IconExtensions)
				{
					FString TestPath = FString::Printf(TEXT("%s/%s/%s/apps/%s%s"), 
						*BasePath, *Theme, *Size, *IconName, *Ext);
					
					if (FPaths::FileExists(TestPath))
					{
						return TestPath;
					}
				}
			}
		}
	}

	for (const FString& BasePath : SearchPaths)
	{
		for (const FString& Ext : IconExtensions)
		{
			FString TestPath = BasePath + TEXT("/") + IconName + Ext;
			
			if (FPaths::FileExists(TestPath))
			{
				return TestPath;
			}
		}
	}

#elif PLATFORM_WINDOWS
	if (FPaths::FileExists(IconName + TEXT(".ico")))
	{
		return IconName + TEXT(".ico");
	}
#endif

	return IconName;
}

FString UDesktopParser::ResolveExecutablePath(const FString& ExecutableName)
{
	if (ExecutableName.IsEmpty())
	{
		return FString();
	}

	if (FPaths::FileExists(ExecutableName))
	{
		return ExecutableName;
	}

	if (ExecutableName.Contains(TEXT("/")) || ExecutableName.Contains(TEXT("\\")))
	{
		return ExecutableName;
	}

#if PLATFORM_LINUX || PLATFORM_MAC
	FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
	TArray<FString> PathDirs;
	PathEnv.ParseIntoArray(PathDirs, TEXT(":"), true);

	for (const FString& Dir : PathDirs)
	{
		FString FullPath = FPaths::Combine(Dir, ExecutableName);
		
		if (FPaths::FileExists(FullPath))
		{
			return FullPath;
		}
	}

	TArray<FString> CommonBinPaths = {
		TEXT("/usr/bin"),
		TEXT("/usr/local/bin"),
		TEXT("/bin"),
		TEXT("/usr/games"),
		TEXT("/snap/bin"),
		TEXT("/var/lib/flatpak/exports/bin")
	};

	FString HomeDir = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
	CommonBinPaths.Add(HomeDir + TEXT("/.local/bin"));

	for (const FString& Dir : CommonBinPaths)
	{
		FString FullPath = FPaths::Combine(Dir, ExecutableName);
		
		if (FPaths::FileExists(FullPath))
		{
			return FullPath;
		}
	}

#elif PLATFORM_WINDOWS
	FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
	TArray<FString> PathDirs;
	PathEnv.ParseIntoArray(PathDirs, TEXT(";"), true);

	TArray<FString> Extensions = { TEXT(".exe"), TEXT(".bat"), TEXT(".cmd"), TEXT(".com") };

	for (const FString& Dir : PathDirs)
	{
		for (const FString& Ext : Extensions)
		{
			FString FullPath = FPaths::Combine(Dir, ExecutableName + Ext);
			
			if (FPaths::FileExists(FullPath))
			{
				return FullPath;
			}
		}

		FString FullPath = FPaths::Combine(Dir, ExecutableName);
		if (FPaths::FileExists(FullPath))
		{
			return FullPath;
		}
	}
#endif

	return ExecutableName;
}

FDesktopEntryInfo UDesktopParser::ParseLinuxDesktopFile(const FString& FilePath)
{
	FDesktopEntryInfo Entry;
	Entry.FilePath = FilePath;

	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		return Entry;
	}

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines);

	FString ExecLine;
	bool bIsApplication = false;
	bool bNoDisplay = false;

	for (const FString& Line : Lines)
	{
		FString TrimmedLine = Line.TrimStartAndEnd();

		if (TrimmedLine.StartsWith(TEXT("Type=")))
		{
			FString Type = TrimmedLine.RightChop(5);
			bIsApplication = Type.Equals(TEXT("Application"), ESearchCase::IgnoreCase);
		}
		else if (TrimmedLine.StartsWith(TEXT("Name=")))
		{
			Entry.Name = TrimmedLine.RightChop(5);
		}
		else if (TrimmedLine.StartsWith(TEXT("Exec=")))
		{
			ExecLine = TrimmedLine.RightChop(5);
			ExecLine.ReplaceInline(TEXT("%F"), TEXT(""));
			ExecLine.ReplaceInline(TEXT("%f"), TEXT(""));
			ExecLine.ReplaceInline(TEXT("%U"), TEXT(""));
			ExecLine.ReplaceInline(TEXT("%u"), TEXT(""));
			ExecLine.ReplaceInline(TEXT("%c"), TEXT(""));
			ExecLine.ReplaceInline(TEXT("%k"), TEXT(""));
			ExecLine = ExecLine.TrimStartAndEnd();
		}
		else if (TrimmedLine.StartsWith(TEXT("Path=")))
		{
			Entry.WorkingDirectory = TrimmedLine.RightChop(5);
		}
		else if (TrimmedLine.StartsWith(TEXT("Icon=")))
		{
			Entry.IconPath = TrimmedLine.RightChop(5);
		}
		else if (TrimmedLine.StartsWith(TEXT("Comment=")))
		{
			Entry.Comment = TrimmedLine.RightChop(8);
		}
		else if (TrimmedLine.StartsWith(TEXT("NoDisplay=")))
		{
			FString Value = TrimmedLine.RightChop(10);
			bNoDisplay = Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
		}
	}

	if (!bIsApplication || bNoDisplay || Entry.Name.IsEmpty() || ExecLine.IsEmpty())
	{
		return Entry;
	}

	SplitCommandLine(ExecLine, Entry.ExecutablePath, Entry.Arguments);
	
	if (Entry.ExecutablePath.Equals(TEXT("env"), ESearchCase::IgnoreCase) && Entry.Arguments.Num() > 0)
	{
		int32 RealExecIndex = -1;
		for (int32 i = 0; i < Entry.Arguments.Num(); i++)
		{
			if (!Entry.Arguments[i].Contains(TEXT("=")))
			{
				RealExecIndex = i;
				break;
			}
		}
		
		if (RealExecIndex >= 0)
		{
			Entry.ExecutablePath = Entry.Arguments[RealExecIndex];
			Entry.Arguments.RemoveAt(0, RealExecIndex + 1);
		}
	}
	
	Entry.ExecutablePath = ResolveExecutablePath(Entry.ExecutablePath);
	
	if (!Entry.IconPath.IsEmpty())
	{
		Entry.IconPath = ResolveIconPath(Entry.IconPath);
	}
	
	Entry.ExecutableName = GetExecutableNameFromPath(Entry.ExecutablePath);
	Entry.bIsValid = !Entry.ExecutablePath.IsEmpty();

	return Entry;
}

FDesktopEntryInfo UDesktopParser::ParseWindowsShortcut(const FString& FilePath)
{
	FDesktopEntryInfo Entry;
	Entry.FilePath = FilePath;

#if PLATFORM_WINDOWS
	CoInitialize(NULL);

	IShellLink* pShellLink = nullptr;
	IPersistFile* pPersistFile = nullptr;

	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);
	
	if (SUCCEEDED(hr))
	{
		hr = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
		
		if (SUCCEEDED(hr))
		{
			hr = pPersistFile->Load(*FilePath, STGM_READ);
			
			if (SUCCEEDED(hr))
			{
				TCHAR TargetPath[MAX_PATH];
				TCHAR Arguments[MAX_PATH];
				TCHAR WorkingDir[MAX_PATH];
				TCHAR Description[MAX_PATH];
				TCHAR IconPath[MAX_PATH];
				int IconIndex;

				if (SUCCEEDED(pShellLink->GetPath(TargetPath, MAX_PATH, NULL, SLGP_RAWPATH)))
				{
					Entry.ExecutablePath = FString(TargetPath);
				}

				if (SUCCEEDED(pShellLink->GetArguments(Arguments, MAX_PATH)))
				{
					FString ArgsString(Arguments);
					if (!ArgsString.IsEmpty())
					{
						Entry.Arguments = TokenizeCommandLine(ArgsString);
					}
				}

				if (SUCCEEDED(pShellLink->GetWorkingDirectory(WorkingDir, MAX_PATH)))
				{
					Entry.WorkingDirectory = FString(WorkingDir);
				}

				if (SUCCEEDED(pShellLink->GetDescription(Description, MAX_PATH)))
				{
					Entry.Comment = FString(Description);
				}

				if (SUCCEEDED(pShellLink->GetIconLocation(IconPath, MAX_PATH, &IconIndex)))
				{
					Entry.IconPath = FString(IconPath);
				}

				Entry.Name = FPaths::GetBaseFilename(FilePath);
				Entry.ExecutableName = GetExecutableNameFromPath(Entry.ExecutablePath);
				Entry.bIsValid = !Entry.ExecutablePath.IsEmpty();
			}

			pPersistFile->Release();
		}

		pShellLink->Release();
	}

	CoUninitialize();
#endif

	return Entry;
}

void UDesktopParser::SplitCommandLine(const FString& FullCommandLine, FString& OutExecutable, TArray<FString>& OutArguments)
{
	OutExecutable.Empty();
	OutArguments.Empty();

	TArray<FString> Tokens = TokenizeCommandLine(FullCommandLine);

	if (Tokens.Num() > 0)
	{
		OutExecutable = Tokens[0];

		for (int32 i = 1; i < Tokens.Num(); i++)
		{
			OutArguments.Add(Tokens[i]);
		}
	}
}

TArray<FString> UDesktopParser::TokenizeCommandLine(const FString& CommandLine)
{
	TArray<FString> Tokens;

	if (CommandLine.IsEmpty())
	{
		return Tokens;
	}

	FString CurrentToken;
	bool bInQuotes = false;
	bool bEscaped = false;

	for (int32 i = 0; i < CommandLine.Len(); i++)
	{
		TCHAR C = CommandLine[i];

		if (bEscaped)
		{
			CurrentToken.AppendChar(C);
			bEscaped = false;
			continue;
		}

		if (C == '\\')
		{
			bEscaped = true;
			continue;
		}

		if (C == '"' || C == '\'')
		{
			bInQuotes = !bInQuotes;
			continue;
		}

		if (FChar::IsWhitespace(C) && !bInQuotes)
		{
			if (!CurrentToken.IsEmpty())
			{
				Tokens.Add(CurrentToken);
				CurrentToken.Empty();
			}
			continue;
		}

		CurrentToken.AppendChar(C);
	}

	if (!CurrentToken.IsEmpty())
	{
		Tokens.Add(CurrentToken);
	}

	return Tokens;
}