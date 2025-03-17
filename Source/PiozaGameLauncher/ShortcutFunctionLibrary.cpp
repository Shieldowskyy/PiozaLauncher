#include "ShortcutFunctionLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h" // Windows specific header
#include <shlobj.h>
#include <Windows.h> // Windows.h header
#include "HAL/PlatformFilemanager.h" // Include for Windows
#endif

#include <string>
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Define TRUE and FALSE for safety
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#if PLATFORM_WINDOWS
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

#endif
// Helper function to create the shortcut path
std::wstring GetShortcutPath(FString Folder, FString ShortcutName)
{
    #if PLATFORM_WINDOWS
    return std::wstring(TCHAR_TO_WCHAR(*Folder)) + L"\\" + std::wstring(*ShortcutName) + L".lnk";
    #else
    return L"";  // Return an empty string for Linux
    #endif
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
    // On Linux, just print a message and return true without doing anything.
    UE_LOG(LogTemp, Warning, TEXT("Linux shortcut creation skipped."));
    return true;
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
    // On Linux, just print a message and return true without doing anything.
    UE_LOG(LogTemp, Warning, TEXT("Linux shortcut removal skipped."));
    return true;
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
    // On Linux, just print a message and return true without doing anything.
    UE_LOG(LogTemp, Warning, TEXT("Linux start menu shortcut removal skipped."));
    return true;
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
    // On Linux, just print a message and return true without doing anything.
    UE_LOG(LogTemp, Warning, TEXT("Linux start menu shortcut creation skipped."));
    return true;
    #endif
}
