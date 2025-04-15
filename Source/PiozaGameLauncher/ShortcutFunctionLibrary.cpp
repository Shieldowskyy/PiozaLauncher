#include "ShortcutFunctionLibrary.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h" // Windows specific header
#include <shlobj.h>
#include <Windows.h> // Windows.h header
#include "HAL/PlatformFilemanager.h" // Include for Windows
#elif PLATFORM_LINUX
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
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

// Helper function to get the shortcut path
#if PLATFORM_WINDOWS
std::wstring GetShortcutPath(FString Folder, FString ShortcutName)
{
    return std::wstring(TCHAR_TO_WCHAR(*Folder)) + L"\\" + std::wstring(TCHAR_TO_WCHAR(*ShortcutName)) + L".lnk";
}
#elif PLATFORM_LINUX
FString GetShortcutPath(FString Folder, FString ShortcutName)
{
    return Folder / (ShortcutName + TEXT(".desktop"));
}
#endif

#if PLATFORM_LINUX
// Helper function to get the user's home directory
FString GetHomeDirectory()
{
    struct passwd* pw = getpwuid(getuid());
    return pw ? FString(UTF8_TO_TCHAR(pw->pw_dir)) : TEXT("~");
}

// Helper function to create a .desktop file
bool CreateDesktopFile(FString FilePath, FString ProgramPath, FString ShortcutName)
{
    FString DesktopFileContent = FString::Printf(
        TEXT("[Desktop Entry]\n")
        TEXT("Version=1.0\n")
        TEXT("Type=Application\n")
        TEXT("Name=%s\n")
        TEXT("Exec=\"%s\"\n")
        TEXT("Terminal=false\n")
        TEXT("Categories=Application;\n"),
        *ShortcutName,
        *ProgramPath
    );

    if (!FFileHelper::SaveStringToFile(DesktopFileContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create .desktop file at %s"), *FilePath);
        return false;
    }

    // Set executable permissions for the .desktop file
    FString Command = FString::Printf(TEXT("chmod +x \"%s\""), *FilePath);
    if (system(TCHAR_TO_UTF8(*Command)) != 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to set executable permissions for %s"), *FilePath);
    }

    return true;
}
#endif

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
    FString DesktopPath = GetHomeDirectory() / TEXT("Desktop");
    FString ShortcutPath = GetShortcutPath(DesktopPath, ShortcutName);

    if (!CreateDesktopFile(ShortcutPath, ProgramPath, ShortcutName))
    {
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Created desktop shortcut at %s"), *ShortcutPath);
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
    FString DesktopPath = GetHomeDirectory() / TEXT("Desktop");
    FString ShortcutPath = GetShortcutPath(DesktopPath, ShortcutName);

    if (!IPlatformFile::GetPlatformPhysical().DeleteFile(*ShortcutPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to remove desktop shortcut at %s"), *ShortcutPath);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Removed desktop shortcut at %s"), *ShortcutPath);
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
    FString ApplicationsPath = GetHomeDirectory() / TEXT(".local/share/applications");
    FString ShortcutPath = GetShortcutPath(ApplicationsPath, ShortcutName);

    if (!CreateDesktopFile(ShortcutPath, ProgramPath, ShortcutName))
    {
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Created start menu shortcut at %s"), *ShortcutPath);
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
    FString ApplicationsPath = GetHomeDirectory() / TEXT(".local/share/applications");
    FString ShortcutPath = GetShortcutPath(ApplicationsPath, ShortcutName);

    if (!IPlatformFile::GetPlatformPhysical().DeleteFile(*ShortcutPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to remove start menu shortcut at %s"), *ShortcutPath);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Removed start menu shortcut at %s"), *ShortcutPath);
    return true;
    #endif
}