// Copyright 2017-2020 Firefly Studio. All Rights Reserved.

#include "EFDCore.h"

#if PLATFORM_WINDOWS
#include "shlobj.h"
#include <Runtime\Core\Public\Windows\COMPointer.h>
#endif

#include <Runtime/Core/Public/HAL/FileManager.h>
#include <Runtime/Core/Public/Misc/Paths.h>

#if PLATFORM_LINUX
#include <Runtime/Core/Public/HAL/PlatformProcess.h>
#endif

#define MAX_FILENAME_STR 65536

bool EFDCore::OpenFileDialogCore(const FString& Title, const FString& Path, const FString& File, const FString& Types, uint32 Flags, TArray<FString>& OutFiles)
{
    int OutFilterIndex = 0;
    return FileDialogShared(false, nullptr, Title, Path, File, Types, Flags, OutFiles, OutFilterIndex);
}

bool EFDCore::SaveFileDialogCore(const FString& Title, const FString& Path, const FString& File, const FString& Types, uint32 Flags, TArray<FString>& OutFiles)
{
    int OutFilterIndex = 0;
    return FileDialogShared(true, nullptr, Title, Path, File, Types, Flags, OutFiles, OutFilterIndex);
}

bool EFDCore::OpenFolderDialogCore(const FString& Title, const FString& Path, FString& OutFolder)
{
    return OpenFolderDialogInner(nullptr, Title, Path, OutFolder);
}

bool EFDCore::FileDialogShared(bool bSave, const void* ParentHandle, const FString& Title, const FString& Path, const FString& File, const FString& Types, uint32 Flags, TArray<FString>& OutFiles, int32& OutFilterIndex)
{
    #if PLATFORM_WINDOWS
    WCHAR Filename[MAX_FILENAME_STR];
    FCString::Strcpy(Filename, MAX_FILENAME_STR, *(File.Replace(TEXT("/"), TEXT("\\"))));
    WCHAR Pathname[MAX_FILENAME_STR];
    FCString::Strcpy(Pathname, MAX_FILENAME_STR, *(FPaths::ConvertRelativePathToFull(Path).Replace(TEXT("/"), TEXT("\\"))));

    WCHAR FileTypeStr[4096];
    FCString::Strcpy(FileTypeStr, 4096, *Types);
    // Replace '|' with '\0' for Windows filter format
    for (int i = 0; FileTypeStr[i] != 0; i++) if (FileTypeStr[i] == '|') FileTypeStr[i] = 0;

    OPENFILENAME ofn{};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = (HWND)ParentHandle;
    ofn.lpstrFilter = FileTypeStr;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = Filename;
    ofn.nMaxFile = MAX_FILENAME_STR;
    ofn.lpstrInitialDir = Pathname;
    ofn.lpstrTitle = *Title;
    ofn.Flags = OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_EXPLORER | (bSave ? (OFN_OVERWRITEPROMPT | OFN_CREATEPROMPT) : (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST));
    if (Flags & EEasyFileDialogFlags::Multiple) ofn.Flags |= OFN_ALLOWMULTISELECT;

    bool bSuccess = bSave ? !!::GetSaveFileName(&ofn) : !!::GetOpenFileName(&ofn);
    if (!bSuccess) return false;

    // Obsługa wielu plików
    FString DirOrFile = FString(Filename);
    TCHAR* pos = Filename + DirOrFile.Len() + 1;
    if ((Flags & EEasyFileDialogFlags::Multiple) && *pos != 0)
    {
        while (*pos)
        {
            OutFiles.Add(DirOrFile / FString(pos));
            pos += FCString::Strlen(pos) + 1;
        }
    }
    else
    {
        OutFiles.Add(DirOrFile);
    }
    OutFilterIndex = ofn.nFilterIndex - 1;
    return true;

    #elif PLATFORM_LINUX
    FString Command = TEXT("zenity --file-selection");
    if (bSave) Command += TEXT(" --save --confirm-overwrite");
    else if (Flags & EEasyFileDialogFlags::Multiple) Command += TEXT(" --multiple");

    if (!Title.IsEmpty()) Command += TEXT(" --title='") + Title + TEXT("'");
    if (!Path.IsEmpty()) Command += TEXT(" --filename='") + FPaths::ConvertRelativePathToFull(Path) + TEXT("'");
    if (!File.IsEmpty() && !bSave) Command += TEXT("/") + File + TEXT("'");

    FString Result, Errors;
    int32 ReturnCode;
    bool bSuccess = FPlatformProcess::ExecProcess(TEXT("/bin/sh"), *(TEXT("-c \"") + Command + TEXT("\"")), &ReturnCode, &Result, &Errors);

    if (!bSuccess || ReturnCode != 0 || Result.IsEmpty()) return false;

    if (Flags & EEasyFileDialogFlags::Multiple)
    {
        TArray<FString> Files;
        Result.ParseIntoArray(Files, TEXT("|"), true);
        for (const FString& f : Files)
            OutFiles.Add(f);
    }
    else
    {
        OutFiles.Add(Result.TrimEnd());
    }
    OutFilterIndex = 0;
    return true;
    #else
    return false;
    #endif
}

bool EFDCore::OpenFolderDialogInner(const void* ParentHandle, const FString& Title, const FString& Path, FString& OutFolder)
{
    #if PLATFORM_WINDOWS
    bool bSuccess = false;
    TComPtr<IFileOpenDialog> Dialog;
    if (SUCCEEDED(::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&Dialog))))
    {
        DWORD Flags = 0;
        Dialog->GetOptions(&Flags);
        Dialog->SetOptions(Flags | FOS_PICKFOLDERS);
        Dialog->SetTitle(*Title);
        if (!Path.IsEmpty())
        {
            FString FullPath = FPaths::ConvertRelativePathToFull(Path);
            TComPtr<IShellItem> PathItem;
            if (SUCCEEDED(::SHCreateItemFromParsingName(*FullPath, nullptr, IID_PPV_ARGS(&PathItem))))
                Dialog->SetFolder(PathItem);
        }
        if (SUCCEEDED(Dialog->Show((HWND)ParentHandle)))
        {
            TComPtr<IShellItem> Result;
            if (SUCCEEDED(Dialog->GetResult(&Result)))
            {
                PWSTR FilePath = nullptr;
                if (SUCCEEDED(Result->GetDisplayName(SIGDN_FILESYSPATH, &FilePath)))
                {
                    OutFolder = FilePath;
                    FPaths::NormalizeDirectoryName(OutFolder);
                    ::CoTaskMemFree(FilePath);
                    bSuccess = true;
                }
            }
        }
    }
    return bSuccess;

    #elif PLATFORM_LINUX
    FString Cmd = TEXT("zenity --file-selection --directory");
    if (!Title.IsEmpty()) Cmd += TEXT(" --title='") + Title + TEXT("'");
    if (!Path.IsEmpty()) Cmd += TEXT(" --filename='") + FPaths::ConvertRelativePathToFull(Path) + TEXT("'");

    FString Result, Errors;
    int32 RetCode;
    bool bSuccess = FPlatformProcess::ExecProcess(TEXT("/bin/sh"), *(TEXT("-c \"") + Cmd + TEXT("\"")), &RetCode, &Result, &Errors);
    if (bSuccess && RetCode == 0 && !Result.IsEmpty())
    {
        OutFolder = Result.TrimEnd();
        FPaths::NormalizeDirectoryName(OutFolder);
        return true;
    }
    return false;
    #else
    return false;
    #endif
}
