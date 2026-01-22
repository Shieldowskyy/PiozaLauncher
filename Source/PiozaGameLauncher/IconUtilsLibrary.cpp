// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "IconUtilsLibrary.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Engine/Texture2D.h"
#include "Modules/ModuleManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <shlobj.h>
#include <shellapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

TArray<FString> UIconUtilsLibrary::GetIconSearchPaths()
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

FString UIconUtilsLibrary::ResolveIconPath(const FString& IconName, int32 PreferredSize)
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

#if PLATFORM_WINDOWS
void* UIconUtilsLibrary::GetWindowsIconHandle(const FString& FilePath, int32 IconIndex)
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

UTexture2D* UIconUtilsLibrary::CreateTextureFromHIcon(void* InHIcon)
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

bool UIconUtilsLibrary::SaveTextureToFile(UTexture2D* Texture, const FString& FilePath)
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

UTexture2D* UIconUtilsLibrary::LoadIconAsTexture(const FString& IconPath, int32 IconIndex)
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
