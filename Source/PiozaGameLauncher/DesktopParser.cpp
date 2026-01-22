// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "DesktopParser.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Async/ParallelFor.h"
#include "HAL/PlatformProcess.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Engine/Texture2D.h"
#include "Modules/ModuleManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shlobj.h>
#include <shellapi.h>
#include "Windows/HideWindowsPlatformTypes.h"

typedef UINT(WINAPI* PFN_PrivateExtractIconsW)(LPCWSTR, int, int, int, HICON*, UINT*, UINT, UINT);
#endif

TArray<FString> UDesktopParser::GetDefaultSearchPaths()
{
	TArray<FString> Paths;

#if PLATFORM_LINUX
	FString Home = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
	Paths.Add(Home + TEXT("/.local/share/applications"));
	Paths.Add(Home + TEXT("/.local/share/flatpak/exports/share/applications"));
	Paths.Add(TEXT("/usr/share/applications"));
	Paths.Add(TEXT("/usr/local/share/applications"));
	Paths.Add(TEXT("/var/lib/flatpak/exports/share/applications"));
	Paths.Add(TEXT("/var/lib/snapd/desktop/applications"));
#elif PLATFORM_WINDOWS
	auto AddPath = [&](int CSIDL) {
		TCHAR Buffer[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL, NULL, 0, Buffer))) {
			Paths.Add(FString(Buffer));
		}
	};
	
	AddPath(CSIDL_PROGRAMS);        // User Start Menu/Programs
	AddPath(CSIDL_COMMON_PROGRAMS); // All Users Start Menu/Programs
	
	TCHAR AppData[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, AppData)))
		Paths.Add(FString(AppData) + TEXT("\\Microsoft\\Windows\\Start Menu\\Programs"));
#endif

	return Paths;
}

TArray<FString> UDesktopParser::GetAllDesktopFiles(const TArray<FString>& SearchPaths)
{
	TArray<FString> FoundFiles;
	IFileManager& FileManager = IFileManager::Get();

#if PLATFORM_LINUX
	const FString Ext = TEXT("*.desktop");
#elif PLATFORM_WINDOWS
	const FString Ext = TEXT("*.lnk");
#else
	return FoundFiles;
#endif

	for (const FString& Path : SearchPaths)
	{
		if (FileManager.DirectoryExists(*Path))
		{
			FileManager.FindFilesRecursive(FoundFiles, *Path, *Ext, true, false, false);
		}
	}
	return FoundFiles;
}

FDesktopEntryInfo UDesktopParser::ParseDesktopFile(const FString& FilePath)
{
	if (!FPaths::FileExists(FilePath)) return FDesktopEntryInfo();

#if PLATFORM_LINUX
	return ParseLinuxDesktopFile(FilePath);
#elif PLATFORM_WINDOWS
	return ParseWindowsShortcut(FilePath);
#else
	return FDesktopEntryInfo();
#endif
}

TArray<FDesktopEntryInfo> UDesktopParser::ParseMultipleDesktopFiles(const TArray<FString>& FilePaths)
{
	TArray<FDesktopEntryInfo> Entries;
	Entries.SetNum(FilePaths.Num());

#if PLATFORM_WINDOWS
	for (int32 i = 0; i < FilePaths.Num(); ++i) Entries[i] = ParseDesktopFile(FilePaths[i]);
#else
	ParallelFor(FilePaths.Num(), [&](int32 i) { Entries[i] = ParseDesktopFile(FilePaths[i]); });
#endif

	return Entries.FilterByPredicate([](const FDesktopEntryInfo& E) { return E.bIsValid; });
}

FString UDesktopParser::GetExecutableNameFromPath(const FString& ExecutablePath)
{
	return FPaths::GetCleanFilename(ExecutablePath);
}

TArray<FString> UDesktopParser::GetIconSearchPaths()
{
	TArray<FString> Paths;
#if PLATFORM_LINUX
	FString Home = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
	Paths.Append({
		Home + TEXT("/.local/share/icons"),
		Home + TEXT("/.icons"),
		TEXT("/usr/share/icons"),
		TEXT("/usr/share/pixmaps"),
		TEXT("/var/lib/flatpak/exports/share/icons"),
		Home + TEXT("/.local/share/flatpak/exports/share/icons")
	});
#elif PLATFORM_WINDOWS
	TCHAR SysPath[MAX_PATH];
	if (GetSystemDirectory(SysPath, MAX_PATH))
		Paths.Add(FString(SysPath) + TEXT("\\..\\..\\Program Files"));
#endif
	return Paths;
}

FString UDesktopParser::ResolveIconPath(const FString& IconName, int32 PreferredSize)
{
	if (IconName.IsEmpty()) return TEXT("");
	if (FPaths::FileExists(IconName)) return IconName;

#if PLATFORM_LINUX
	if (IconName.StartsWith(TEXT("/"))) return TEXT("");

	TArray<FString> Exts = { TEXT(".png"), TEXT(".svg"), TEXT(".xpm"), TEXT(".jpg") };
	auto CheckExists = [&](const FString& Path) { return FPaths::FileExists(Path); };

	// Check for direct extension match in search paths
	for (const FString& Base : GetIconSearchPaths())
	{
		for (const FString& Ext : Exts)
		{
			FString Path = Base / IconName + Ext;
			if (CheckExists(Path)) return Path;
		}
	}

	// Complex theme search
	TArray<FString> Themes = { TEXT("hicolor"), TEXT("Adwaita"), TEXT("breeze"), TEXT("default") };
	TArray<FString> Sizes = { FString::FromInt(PreferredSize) + TEXT("x") + FString::FromInt(PreferredSize), TEXT("scalable"), TEXT("256x256"), TEXT("48x48"), TEXT("32x32") };

	for (const FString& Base : GetIconSearchPaths())
	for (const FString& Theme : Themes)
	for (const FString& Size : Sizes)
	for (const FString& Ext : Exts)
	{
		FString Path = FString::Printf(TEXT("%s/%s/%s/apps/%s%s"), *Base, *Theme, *Size, *IconName, *Ext);
		if (CheckExists(Path)) return Path;
	}

#elif PLATFORM_WINDOWS
	TCHAR Expanded[MAX_PATH];
	if (ExpandEnvironmentStrings(*IconName, Expanded, MAX_PATH))
	{
		FString Res = Expanded;
		if (FPaths::FileExists(Res)) return Res;
		if (FPaths::FileExists(Res + TEXT(".ico"))) return Res + TEXT(".ico");
		if (FPaths::FileExists(Res + TEXT(".exe"))) return Res + TEXT(".exe");
		if (FPaths::FileExists(Res + TEXT(".dll"))) return Res + TEXT(".dll");
	}
#endif
	return IconName;
}

FString UDesktopParser::ResolveExecutablePath(const FString& ExecutableName)
{
	if (ExecutableName.IsEmpty() || FPaths::FileExists(ExecutableName)) return ExecutableName;
	if (ExecutableName.Contains(TEXT("/")) || ExecutableName.Contains(TEXT("\\"))) return ExecutableName;

	FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
	TArray<FString> Dirs;
#if PLATFORM_LINUX
	PathEnv.ParseIntoArray(Dirs, TEXT(":"), true);
	Dirs.Append({ TEXT("/usr/bin"), TEXT("/usr/local/bin"), TEXT("/snap/bin"), TEXT("/var/lib/flatpak/exports/bin") });
	Dirs.Add(FPlatformMisc::GetEnvironmentVariable(TEXT("HOME")) + TEXT("/.local/bin"));
	
	for (const FString& Dir : Dirs)
	{
		FString Full = Dir / ExecutableName;
		if (FPaths::FileExists(Full)) return Full;
	}
#elif PLATFORM_WINDOWS
	PathEnv.ParseIntoArray(Dirs, TEXT(";"), true);
	TArray<FString> Exts = { TEXT(".exe"), TEXT(".bat"), TEXT(".cmd") };
	for (const FString& Dir : Dirs)
	{
		for (const FString& Ext : Exts)
		{
			if (FPaths::FileExists(Dir / ExecutableName + Ext)) return Dir / ExecutableName + Ext;
		}
	}
#endif
	return ExecutableName;
}

FDesktopEntryInfo UDesktopParser::ParseLinuxDesktopFile(const FString& FilePath)
{
	FDesktopEntryInfo Entry;
	Entry.FilePath = FilePath;

	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *FilePath)) return Entry;

	TArray<FString> Lines;
	Content.ParseIntoArrayLines(Lines);

	bool bIsApp = false;
	FString ExecLine;

	for (const FString& Line : Lines)
	{
		FString Trim = Line.TrimStartAndEnd();
		if (Trim.StartsWith(TEXT("Type=Application"))) bIsApp = true;
		else if (Trim.StartsWith(TEXT("Name="))) Entry.Name = Trim.RightChop(5);
		else if (Trim.StartsWith(TEXT("Exec="))) ExecLine = Trim.RightChop(5);
		else if (Trim.StartsWith(TEXT("Path="))) Entry.WorkingDirectory = Trim.RightChop(5);
		else if (Trim.StartsWith(TEXT("Icon="))) Entry.IconPath = Trim.RightChop(5);
		else if (Trim.StartsWith(TEXT("Comment="))) Entry.Comment = Trim.RightChop(8);
		else if (Trim.StartsWith(TEXT("NoDisplay=true"))) return Entry;
	}

	if (!bIsApp || Entry.Name.IsEmpty() || ExecLine.IsEmpty()) return Entry;

	static const TArray<FString> Codes = { TEXT("%F"), TEXT("%f"), TEXT("%U"), TEXT("%u"), TEXT("%c"), TEXT("%k") };
	for (const FString& Code : Codes) ExecLine.ReplaceInline(*Code, TEXT(""));
	ExecLine.TrimStartAndEndInline();

	SplitCommandLine(ExecLine, Entry.ExecutablePath, Entry.Arguments);
	
	if (Entry.ExecutablePath.Equals(TEXT("env"), ESearchCase::IgnoreCase) && Entry.Arguments.Num() > 0)
	{
		int32 Idx = Entry.Arguments.IndexOfByPredicate([](const FString& A){ return !A.Contains(TEXT("=")); });
		if (Idx != INDEX_NONE)
		{
			Entry.ExecutablePath = Entry.Arguments[Idx];
			Entry.Arguments.RemoveAt(0, Idx + 1);
		}
	}

	Entry.ExecutablePath = ResolveExecutablePath(Entry.ExecutablePath);
	if (!Entry.IconPath.IsEmpty()) Entry.IconPath = ResolveIconPath(Entry.IconPath);
	Entry.ExecutableName = GetExecutableNameFromPath(Entry.ExecutablePath);
	Entry.bIsValid = !Entry.ExecutablePath.IsEmpty();

	return Entry;
}

FDesktopEntryInfo UDesktopParser::ParseWindowsShortcut(const FString& FilePath)
{
	FDesktopEntryInfo Entry;
	Entry.FilePath = FilePath;

#if PLATFORM_WINDOWS
	HRESULT hr = CoInitialize(NULL);
	IShellLink* pLink = nullptr;
	
	if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&pLink)))
	{
		IPersistFile* pFile = nullptr;
		if (SUCCEEDED(pLink->QueryInterface(IID_IPersistFile, (void**)&pFile)))
		{
			if (SUCCEEDED(pFile->Load(*FilePath, STGM_READ)))
			{
				TCHAR Buf[MAX_PATH];
				if (SUCCEEDED(pLink->GetPath(Buf, MAX_PATH, NULL, 0))) Entry.ExecutablePath = Buf;
				if (SUCCEEDED(pLink->GetArguments(Buf, MAX_PATH))) Entry.Arguments = TokenizeCommandLine(Buf);
				if (SUCCEEDED(pLink->GetWorkingDirectory(Buf, MAX_PATH))) Entry.WorkingDirectory = Buf;
				if (SUCCEEDED(pLink->GetDescription(Buf, MAX_PATH))) Entry.Comment = Buf;

				int IconIdx = 0;
				if (SUCCEEDED(pLink->GetIconLocation(Buf, MAX_PATH, &IconIdx)))
				{
					Entry.IconPath = Buf;
					Entry.IconIndex = IconIdx;
				}

				if (Entry.IconPath.IsEmpty()) Entry.IconPath = Entry.ExecutablePath;
				Entry.IconPath = ResolveIconPath(Entry.IconPath);
				Entry.Name = FPaths::GetBaseFilename(FilePath);
				Entry.ExecutableName = GetExecutableNameFromPath(Entry.ExecutablePath);
				Entry.bIsValid = !Entry.ExecutablePath.IsEmpty();
			}
			pFile->Release();
		}
		pLink->Release();
	}
	if (SUCCEEDED(hr)) CoUninitialize();
#endif
	return Entry;
}

void UDesktopParser::SplitCommandLine(const FString& FullCommandLine, FString& OutExecutable, TArray<FString>& OutArguments)
{
	TArray<FString> Tokens = TokenizeCommandLine(FullCommandLine);
	if (Tokens.Num() > 0)
	{
		OutExecutable = Tokens[0];
		OutArguments = Tokens;
		OutArguments.RemoveAt(0);
	}
}

TArray<FString> UDesktopParser::TokenizeCommandLine(const FString& CommandLine)
{
	TArray<FString> Tokens;
	FString Token;
	bool bQuote = false;
	bool bEsc = false;

	for (TCHAR C : CommandLine)
	{
		if (bEsc) { Token.AppendChar(C); bEsc = false; }
		else if (C == '\\') bEsc = true;
		else if ((C == '"' || C == '\'')) bQuote = !bQuote;
		else if (FChar::IsWhitespace(C) && !bQuote)
		{
			if (!Token.IsEmpty()) { Tokens.Add(Token); Token.Empty(); }
		}
		else Token.AppendChar(C);
	}
	if (!Token.IsEmpty()) Tokens.Add(Token);
	return Tokens;
}

#if PLATFORM_WINDOWS
void* UDesktopParser::GetWindowsIconHandle(const FString& FilePath, int32 IconIndex)
{
	TCHAR PathW[MAX_PATH];
	if (!ExpandEnvironmentStrings(*FilePath, PathW, MAX_PATH)) return nullptr;

	HICON hIcon = nullptr;
	if (SUCCEEDED(SHDefExtractIcon(PathW, IconIndex, 0, &hIcon, nullptr, 256)))
	{
		return hIcon;
	}

	SHFILEINFO sfi = { 0 };
	if (SHGetFileInfo(PathW, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON))
	{
		return sfi.hIcon;
	}

	return nullptr;
}

UTexture2D* UDesktopParser::CreateTextureFromHIcon(void* InHIcon)
{
	HICON hIcon = (HICON)InHIcon;
	if (!hIcon) return nullptr;

	ICONINFO Info;
	if (!GetIconInfo(hIcon, &Info)) return nullptr;

	// Helper to cleanup GDI objects
	struct FGDIResource {
		HBITMAP Color, Mask;
		HDC DC;
		FGDIResource(ICONINFO& I, HDC D) : Color(I.hbmColor), Mask(I.hbmMask), DC(D) {}
		~FGDIResource() {
			if (DC) DeleteDC(DC);
			if (Color) DeleteObject(Color);
			if (Mask) DeleteObject(Mask);
		}
	};

	HDC hDC = CreateCompatibleDC(NULL);
	FGDIResource Res(Info, hDC);
	if (!hDC) return nullptr;

	BITMAP bm;
	::GetObjectW(Info.hbmColor, sizeof(BITMAP), &bm);
	int Width = bm.bmWidth;
	int Height = bm.bmHeight;

	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = Width;
	bmi.bmiHeader.biHeight = -Height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	TArray<uint8> Pixels;
	Pixels.SetNumUninitialized(Width * Height * 4);

	if (GetDIBits(hDC, Info.hbmColor, 0, Height, Pixels.GetData(), &bmi, DIB_RGB_COLORS))
	{
		UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
		if (Texture)
		{
			Texture->CompressionSettings = TC_EditorIcon;
			Texture->SRGB = true;
			Texture->Filter = TF_Trilinear;
			
			void* Data = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num());
			Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
			Texture->UpdateResource();
			return Texture;
		}
	}
	return nullptr;
}
#endif

bool UDesktopParser::SaveTextureToFile(UTexture2D* Texture, const FString& FilePath)
{
	if (!Texture || !Texture->GetPlatformData())
	{
		UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Invalid texture or platform data."));
		return false;
	}

	int32 Width = Texture->GetSizeX();
	int32 Height = Texture->GetSizeY();

	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	const void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);
	if (!Data)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Failed to lock mip data for %s."), *FilePath);
		return false;
	}

	TArray<uint8> RawData;
	RawData.SetNumUninitialized(Mip.BulkData.GetBulkDataSize());
	FMemory::Memcpy(RawData.GetData(), Data, RawData.Num());
	
	Mip.BulkData.Unlock();

	EImageFormat Format = EImageFormat::JPEG;
	FString FormatName = TEXT("JPEG");
	if (FilePath.EndsWith(TEXT(".png"), ESearchCase::IgnoreCase)) 
	{
		Format = EImageFormat::PNG;
		FormatName = TEXT("PNG");
	}
	else if (FilePath.EndsWith(TEXT(".bmp"), ESearchCase::IgnoreCase))
	{
		Format = EImageFormat::BMP;
		FormatName = TEXT("BMP");
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);
	
	if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(RawData.GetData(), RawData.Num(), Width, Height, ERGBFormat::BGRA, 8))
	{
		const TArray64<uint8>& CompressedData = ImageWrapper->GetCompressed(); 
		TArray<uint8> Output(CompressedData);
		if (FFileHelper::SaveArrayToFile(Output, *FilePath))
		{
			UE_LOG(LogTemp, Log, TEXT("SaveTextureToFile: Successfully saved %dx%d %s to %s."), Width, Height, *FormatName, *FilePath);
			return true;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Failed to save file to %s."), *FilePath);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SaveTextureToFile: Failed to compress texture data for %s."), *FilePath);
	}

	return false;
}

UTexture2D* UDesktopParser::LoadIconAsTexture(const FString& IconPath, int32 IconIndex)
{
	if (!FPaths::FileExists(IconPath)) return nullptr;

#if PLATFORM_WINDOWS
	if (void* hIcon = GetWindowsIconHandle(IconPath, IconIndex))
	{
		UTexture2D* Tex = CreateTextureFromHIcon(hIcon);
		DestroyIcon((HICON)hIcon);
		if (Tex) return Tex;
	}
#endif

	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *IconPath)) return nullptr;

	IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	EImageFormat Format = Module.DetectImageFormat(Data.GetData(), Data.Num());

	if (Format == EImageFormat::Invalid) return nullptr;

	TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(Format);
	if (Wrapper.IsValid() && Wrapper->SetCompressed(Data.GetData(), Data.Num()))
	{
		TArray<uint8> Raw;
		if (Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
		{
			UTexture2D* Tex = UTexture2D::CreateTransient(Wrapper->GetWidth(), Wrapper->GetHeight(), PF_B8G8R8A8);
			if (Tex)
			{
				Tex->CompressionSettings = TC_EditorIcon;
				Tex->SRGB = true;
				Tex->Filter = TF_Trilinear;
				void* TexData = Tex->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TexData, Raw.GetData(), Raw.Num());
				Tex->GetPlatformData()->Mips[0].BulkData.Unlock();
				Tex->UpdateResource();
				return Tex;
			}
		}
	}

	return nullptr;
}