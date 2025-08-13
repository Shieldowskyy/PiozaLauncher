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

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// --- WINDOWS HELPERS ---
#if PLATFORM_WINDOWS

bool InitializeCOM()
{
    return SUCCEEDED(CoInitialize(NULL));
}

void UninitializeCOM()
{
    CoUninitialize();
}

FString GetShortcutPath(const FString& Folder, const FString& ShortcutName)
{
    return Folder / (ShortcutName + TEXT(".lnk"));
}

bool CreateWindowsShortcut(const FString& Folder, const FString& ProgramPath, const FString& ShortcutName, const FString& LaunchArgs)
{
    if (!InitializeCOM())
        return false;

    TCHAR szPath[MAX_PATH];
    UINT csidl = (Folder == TEXT("Desktop")) ? CSIDL_DESKTOPDIRECTORY : CSIDL_PROGRAMS;

    if (FAILED(SHGetSpecialFolderPath(NULL, szPath, csidl, FALSE)))
    {
        UninitializeCOM();
        return false;
    }

    FString ShortcutPath = GetShortcutPath(FString(szPath), ShortcutName);

    IShellLink* pShellLink = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);

    if (FAILED(hr))
    {
        UninitializeCOM();
        return false;
    }

    pShellLink->SetPath(TCHAR_TO_WCHAR(*ProgramPath));

    if (!LaunchArgs.IsEmpty())
        pShellLink->SetArguments(TCHAR_TO_WCHAR(*LaunchArgs));

    IPersistFile* pPersistFile = nullptr;
    hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);

    if (FAILED(hr))
    {
        pShellLink->Release();
        UninitializeCOM();
        return false;
    }

    hr = pPersistFile->Save(*ShortcutPath, TRUE);
    pPersistFile->Release();
    pShellLink->Release();
    UninitializeCOM();

    return SUCCEEDED(hr);
}

bool RemoveWindowsShortcut(const FString& Folder, const FString& ShortcutName)
{
    if (!InitializeCOM())
        return false;

    TCHAR szPath[MAX_PATH];
    UINT csidl = (Folder == TEXT("Desktop")) ? CSIDL_DESKTOPDIRECTORY : CSIDL_PROGRAMS;

    if (FAILED(SHGetSpecialFolderPath(NULL, szPath, csidl, FALSE)))
    {
        UninitializeCOM();
        return false;
    }

    FString ShortcutPath = GetShortcutPath(FString(szPath), ShortcutName);
    bool bResult = DeleteFileW(*ShortcutPath) != 0;

    UninitializeCOM();
    return bResult;
}

#endif // PLATFORM_WINDOWS

// --- LINUX HELPERS ---
#if PLATFORM_LINUX

FString GetHomeDirectory()
{
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir)
        return FString(UTF8_TO_TCHAR(pw->pw_dir));

    const char* home = getenv("HOME");
    return home ? FString(UTF8_TO_TCHAR(home)) : TEXT("~");
}

FString GetShortcutPath(const FString& Folder, const FString& ShortcutName)
{
    return Folder / (ShortcutName + TEXT(".desktop"));
}

bool CreateDesktopFile(const FString& FilePath, const FString& ExecCommand, const FString& ShortcutName, const FString& IconPath)
{
    FString ResolvedIconPath = IconPath.IsEmpty() ? TEXT("application-default-icon") : IconPath;

    FString DesktopFileContent = FString::Printf(
        TEXT("[Desktop Entry]\n"
            "Version=1.0\n"
            "Type=Application\n"
            "Name=%s\n"
            "Exec=%s\n"
            "Icon=%s\n"
            "Terminal=false\n"
            "Categories=Application;\n"),
        *ShortcutName,
        *ExecCommand,
        *ResolvedIconPath
    );

    if (!FFileHelper::SaveStringToFile(DesktopFileContent, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create .desktop file at %s"), *FilePath);
        return false;
    }

    if (chmod(TCHAR_TO_UTF8(*FilePath), 0755) != 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to set executable permission on %s"), *FilePath);
    }

    return true;
}

bool RemoveLinuxShortcut(const FString& Folder, const FString& ShortcutName)
{
    FString ShortcutPath = GetShortcutPath(Folder, ShortcutName);
    return IPlatformFile::GetPlatformPhysical().DeleteFile(*ShortcutPath);
}

#endif // PLATFORM_LINUX

// --- PUBLIC API ---

bool UShortcutFunctionLibrary::CreateDesktopShortcut(const FString& ProgramPath, const FString& ShortcutName, const FString& LaunchArgs, const FString& IconPath)
{
#if PLATFORM_WINDOWS
    return CreateWindowsShortcut(TEXT("Desktop"), ProgramPath, ShortcutName, LaunchArgs);

#elif PLATFORM_LINUX
    FString DesktopPath = GetHomeDirectory() / TEXT("Desktop");
    FString FullCommand = ProgramPath;
    if (!LaunchArgs.IsEmpty())
        FullCommand += TEXT(" ") + LaunchArgs;

    return CreateDesktopFile(DesktopPath / (ShortcutName + TEXT(".desktop")), FullCommand, ShortcutName, IconPath);
#else
    return false;
#endif
}

bool UShortcutFunctionLibrary::RemoveDesktopShortcut(const FString& ShortcutName)
{
#if PLATFORM_WINDOWS
    return RemoveWindowsShortcut(TEXT("Desktop"), ShortcutName);

#elif PLATFORM_LINUX
    FString DesktopPath = GetHomeDirectory() / TEXT("Desktop");
    return RemoveLinuxShortcut(DesktopPath, ShortcutName);
#else
    return false;
#endif
}

bool UShortcutFunctionLibrary::CreateStartMenuShortcut(const FString& ProgramPath, const FString& ShortcutName, const FString& LaunchArgs, const FString& IconPath)
{
#if PLATFORM_WINDOWS
    return CreateWindowsShortcut(TEXT("Programs"), ProgramPath, ShortcutName, LaunchArgs);

#elif PLATFORM_LINUX
    FString ApplicationsPath = GetHomeDirectory() / TEXT(".local/share/applications");
    FString FullCommand = ProgramPath;
    if (!LaunchArgs.IsEmpty())
        FullCommand += TEXT(" ") + LaunchArgs;

    return CreateDesktopFile(ApplicationsPath / (ShortcutName + TEXT(".desktop")), FullCommand, ShortcutName, IconPath);
#else
    return false;
#endif
}

bool UShortcutFunctionLibrary::RemoveStartMenuShortcut(const FString& ShortcutName)
{
#if PLATFORM_WINDOWS
    return RemoveWindowsShortcut(TEXT("Programs"), ShortcutName);

#elif PLATFORM_LINUX
    FString ApplicationsPath = GetHomeDirectory() / TEXT(".local/share/applications");
    return RemoveLinuxShortcut(ApplicationsPath, ShortcutName);
#else
    return false;
#endif
}
