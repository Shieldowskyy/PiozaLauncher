#include "ShortcutFunctionLibrary.h"
#include "Windows/WindowsHWrapper.h" // Updated header
#include <shlobj.h>
#include <string>
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include <Windows.h> // Added Windows.h header

// Define TRUE and FALSE for safety
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// Helper function to initialize and uninitialize COM
bool InitializeCOM()
{
    if (FAILED(CoInitialize(NULL)))
    {
        return false;
    }
    return true;
}

void UninitializeCOM()
{
    CoUninitialize();
}

// Helper function to create the shortcut path
std::wstring GetShortcutPath(FString Folder, FString ShortcutName)
{
    return std::wstring(TCHAR_TO_WCHAR(*Folder)) + L"\\" + std::wstring(*ShortcutName) + L".lnk";
}

// Function to create a shortcut on the desktop
bool UShortcutFunctionLibrary::CreateDesktopShortcut(FString ProgramPath, FString ShortcutName)
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
    HRESULT hr;

    IShellLink* pShellLink = NULL;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);

    if (FAILED(hr))
    {
        UninitializeCOM();
        return false;
    }

    pShellLink->SetPath(TCHAR_TO_WCHAR(*ProgramPath));

    IPersistFile* pPersistFile = NULL;
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
    FString DesktopDir = FPaths::DesktopDir();
    FString ShortcutPath = DesktopDir / ShortcutName + ".desktop";

    FString ShortcutContent =
        "[Desktop Entry]\n"
        "Version=1.0\n"
        "Name=" + ShortcutName + "\n"
        "Comment=Shortcut to " + ProgramPath + "\n"
        "Exec=" + ProgramPath + "\n"
        "Icon=" + ProgramPath + "\n"
        "Terminal=false\n"
        "Type=Application\n";

    return FFileHelper::SaveStringToFile(ShortcutContent, *ShortcutPath);
#endif
}

// Function to remove a shortcut from the desktop
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

    if (DeleteFileW(shortcutPath.c_str()) == 0)
    {
        UninitializeCOM();
        return false;
    }

    UninitializeCOM();
    return true;

#elif PLATFORM_LINUX
    FString DesktopDir = FPaths::DesktopDir();
    FString ShortcutPath = DesktopDir / ShortcutName + ".desktop";

    return IFileManager::Get().Delete(*ShortcutPath);
#endif
}

// Function to remove a shortcut from the Start menu
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

    if (DeleteFileW(shortcutPath.c_str()) == 0)
    {
        UninitializeCOM();
        return false;
    }

    UninitializeCOM();
    return true;

#elif PLATFORM_LINUX
    FString ApplicationsDir = FPaths::Combine(FPaths::ProjectDir(), "Applications");
    FString ShortcutPath = FPaths::Combine(ApplicationsDir, ShortcutName + ".desktop");

    return IFileManager::Get().Delete(*ShortcutPath);
#endif
}

// Function to create a shortcut in the Start menu
bool UShortcutFunctionLibrary::CreateStartMenuShortcut(FString ProgramPath, FString ShortcutName)
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
    HRESULT hr;

    IShellLink* pShellLink = NULL;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);

    if (FAILED(hr))
    {
        UninitializeCOM();
        return false;
    }

    pShellLink->SetPath(TCHAR_TO_WCHAR(*ProgramPath));

    IPersistFile* pPersistFile = NULL;
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
    FString ApplicationsDir = FPaths::Combine(FPaths::ProjectDir(), "Applications");
    FString ShortcutPath = FPaths::Combine(ApplicationsDir, ShortcutName + ".desktop");

    FString ShortcutContent =
        "[Desktop Entry]\n"
        "Version=1.0\n"
        "Name=" + ShortcutName + "\n"
        "Comment=Shortcut to " + ProgramPath + "\n"
        "Exec=" + ProgramPath + "\n"
        "Icon=" + ProgramPath + "\n"
        "Terminal=false\n"
        "Type=Application\n";

    return FFileHelper::SaveStringToFile(ShortcutContent, *ShortcutPath);
#endif
}
