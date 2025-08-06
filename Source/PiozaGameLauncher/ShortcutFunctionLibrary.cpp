#include "ShortcutFunctionLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include <shlobj.h>
#include <Windows.h>
#include "HAL/PlatformFilemanager.h"
#elif PLATFORM_LINUX
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#endif

#include <string>
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#if PLATFORM_WINDOWS
bool InitializeCOM()
{
    return SUCCEEDED(CoInitialize(NULL));
}

void UninitializeCOM()
{
    CoUninitialize();
}

std::wstring GetShortcutPath(FString Folder, FString ShortcutName)
{
    return std::wstring(TCHAR_TO_WCHAR(*Folder)) + L"\\" + std::wstring(TCHAR_TO_WCHAR(*ShortcutName)) + L".lnk";
}
#elif PLATFORM_LINUX
FString GetHomeDirectory()
{
    struct passwd* pw = getpwuid(getuid());
    return pw ? FString(UTF8_TO_TCHAR(pw->pw_dir)) : TEXT("~");
}

FString GetShortcutPath(FString Folder, FString ShortcutName)
{
    return Folder / (ShortcutName + TEXT(".desktop"));
}

bool CreateDesktopFile(FString FilePath, FString ExecCommand, FString ShortcutName, FString IconPath)
{
    FString ResolvedIconPath = IconPath.IsEmpty() ? TEXT("application-default-icon") : IconPath;

    FString DesktopFileContent = FString::Printf(
        TEXT("[Desktop Entry]\n")
        TEXT("Version=1.0\n")
        TEXT("Type=Application\n")
        TEXT("Name=%s\n")
        TEXT("Exec=%s\n")
        TEXT("Icon=%s\n")
        TEXT("Terminal=false\n")
        TEXT("Categories=Application;\n"),
                                                 *ShortcutName,
                                                 *ExecCommand,
                                                 *ResolvedIconPath
    );

    if (!FFileHelper::SaveStringToFile(DesktopFileContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create .desktop file at %s"), *FilePath);
        return false;
    }

    FString Command = FString::Printf(TEXT("chmod +x \"%s\""), *FilePath);
    system(TCHAR_TO_UTF8(*Command));

    return true;
}

#endif

bool UShortcutFunctionLibrary::CreateDesktopShortcut(FString ProgramPath, FString ShortcutName, FString LaunchArgs, FString IconPath)
{
    #if PLATFORM_WINDOWS
    if (!InitializeCOM())
        return false;

    ProgramPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::IgnoreCase);

    TCHAR szPath[MAX_PATH];
    if (FAILED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, FALSE)))
    {
        UninitializeCOM();
        return false;
    }

    std::wstring shortcutPath = GetShortcutPath(FString(szPath), ShortcutName);
    HRESULT hr;

    IShellLink* pShellLink = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);

    if (FAILED(hr))
    {
        UninitializeCOM();
        return false;
    }

    pShellLink->SetPath(TCHAR_TO_WCHAR(*ProgramPath));

    if (!LaunchArgs.IsEmpty())
    {
        pShellLink->SetArguments(TCHAR_TO_WCHAR(*LaunchArgs));
    }

    IPersistFile* pPersistFile = nullptr;
    hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
    if (FAILED(hr))
    {
        pShellLink->Release();
        UninitializeCOM();
        return false;
    }

    hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
    pPersistFile->Release();
    pShellLink->Release();
    UninitializeCOM();

    return SUCCEEDED(hr);

    #elif PLATFORM_LINUX
    FString DesktopPath = GetHomeDirectory() / TEXT("Desktop");
    FString ShortcutPath = GetShortcutPath(DesktopPath, ShortcutName);

    FString FullCommand = ProgramPath;
    if (!LaunchArgs.IsEmpty())
    {
        FullCommand += TEXT(" ") + LaunchArgs;
    }

    return CreateDesktopFile(ShortcutPath, FullCommand, ShortcutName, IconPath);
    #endif
}

bool UShortcutFunctionLibrary::RemoveDesktopShortcut(FString ShortcutName)
{
    #if PLATFORM_WINDOWS
    if (!InitializeCOM())
        return false;

    TCHAR szPath[MAX_PATH];
    if (FAILED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, FALSE)))
    {
        UninitializeCOM();
        return false;
    }

    std::wstring shortcutPath = GetShortcutPath(FString(szPath), ShortcutName);

    bool bResult = DeleteFileW(shortcutPath.c_str()) != 0;

    UninitializeCOM();
    return bResult;

    #elif PLATFORM_LINUX
    FString DesktopPath = GetHomeDirectory() / TEXT("Desktop");
    FString ShortcutPath = GetShortcutPath(DesktopPath, ShortcutName);

    return IPlatformFile::GetPlatformPhysical().DeleteFile(*ShortcutPath);
    #endif
}

bool UShortcutFunctionLibrary::CreateStartMenuShortcut(FString ProgramPath, FString ShortcutName, FString LaunchArgs, FString IconPath)
{
    #if PLATFORM_WINDOWS
    if (!InitializeCOM())
        return false;

    ProgramPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::IgnoreCase);

    TCHAR szPath[MAX_PATH];
    if (FAILED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_PROGRAMS, FALSE)))
    {
        UninitializeCOM();
        return false;
    }

    std::wstring shortcutPath = GetShortcutPath(FString(szPath), ShortcutName);
    HRESULT hr;

    IShellLink* pShellLink = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);

    if (FAILED(hr))
    {
        UninitializeCOM();
        return false;
    }

    pShellLink->SetPath(TCHAR_TO_WCHAR(*ProgramPath));

    if (!LaunchArgs.IsEmpty())
    {
        pShellLink->SetArguments(TCHAR_TO_WCHAR(*LaunchArgs));
    }

    IPersistFile* pPersistFile = nullptr;
    hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
    if (FAILED(hr))
    {
        pShellLink->Release();
        UninitializeCOM();
        return false;
    }

    hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
    pPersistFile->Release();
    pShellLink->Release();
    UninitializeCOM();

    return SUCCEEDED(hr);

    #elif PLATFORM_LINUX
    FString ApplicationsPath = GetHomeDirectory() / TEXT(".local/share/applications");
    FString ShortcutPath = GetShortcutPath(ApplicationsPath, ShortcutName);

    FString FullCommand = ProgramPath;
    if (!LaunchArgs.IsEmpty())
    {
        FullCommand += TEXT(" ") + LaunchArgs;
    }

    return CreateDesktopFile(ShortcutPath, FullCommand, ShortcutName, IconPath);
    #endif
}

bool UShortcutFunctionLibrary::RemoveStartMenuShortcut(FString ShortcutName)
{
    #if PLATFORM_WINDOWS
    if (!InitializeCOM())
        return false;

    TCHAR szPath[MAX_PATH];
    if (FAILED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_PROGRAMS, FALSE)))
    {
        UninitializeCOM();
        return false;
    }

    std::wstring shortcutPath = GetShortcutPath(FString(szPath), ShortcutName);

    bool bResult = DeleteFileW(shortcutPath.c_str()) != 0;

    UninitializeCOM();
    return bResult;

    #elif PLATFORM_LINUX
    FString ApplicationsPath = GetHomeDirectory() / TEXT(".local/share/applications");
    FString ShortcutPath = GetShortcutPath(ApplicationsPath, ShortcutName);

    return IPlatformFile::GetPlatformPhysical().DeleteFile(*ShortcutPath);
    #endif
}
