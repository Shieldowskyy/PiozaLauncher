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

#define MAX_FILETYPES_STR 4096
#define MAX_FILENAME_STR 65536 // Buffer size for file names

bool EFDCore::OpenFileDialogCore(const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
    int OutFilterIndex = 0;
    return FileDialogShared(false, nullptr, DialogTitle, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames, OutFilterIndex);
}

bool EFDCore::SaveFileDialogCore(const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
    int OutFilterIndex = 0;
    return FileDialogShared(true, nullptr, DialogTitle, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames, OutFilterIndex);
}

bool EFDCore::OpenFolderDialogCore(const FString& DialogTitle, const FString& DefaultPath, FString& OutFoldername)
{
    return OpenFolderDialogInner(NULL, DialogTitle, DefaultPath, OutFoldername);
}

bool EFDCore::FileDialogShared(bool bSave, const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames, int32& OutFilterIndex)
{
#pragma region Windows
#if PLATFORM_WINDOWS
    WCHAR Filename[MAX_FILENAME_STR];
    FCString::Strcpy(Filename, MAX_FILENAME_STR, *(DefaultFile.Replace(TEXT("/"), TEXT("\\"))));

    WCHAR Pathname[MAX_FILENAME_STR];
    FCString::Strcpy(Pathname, MAX_FILENAME_STR, *(FPaths::ConvertRelativePathToFull(DefaultPath).Replace(TEXT("/"), TEXT("\\"))));

    WCHAR FileTypeStr[MAX_FILETYPES_STR];
    WCHAR* FileTypesPtr = NULL;
    const int32 FileTypesLen = FileTypes.Len();

    TArray<FString> CleanExtensionList;
    TArray<FString> UnformattedExtensions;
    FileTypes.ParseIntoArray(UnformattedExtensions, TEXT("|"), true);
    for (int32 ExtensionIndex = 1; ExtensionIndex < UnformattedExtensions.Num(); ExtensionIndex += 2)
    {
        const FString& Extension = UnformattedExtensions[ExtensionIndex];
        if (Extension != TEXT("*.*"))
        {
            int32 WildCardIndex = Extension.Find(TEXT("*"));
            CleanExtensionList.Add(WildCardIndex != INDEX_NONE ? Extension.RightChop(WildCardIndex + 1) : Extension);
        }
    }

    if (FileTypesLen > 0 && FileTypesLen - 1 < MAX_FILETYPES_STR)
    {
        FileTypesPtr = FileTypeStr;
        FCString::Strcpy(FileTypeStr, MAX_FILETYPES_STR, *FileTypes);
        TCHAR* Pos = FileTypeStr;
        while (Pos[0] != 0)
        {
            if (Pos[0] == '|') Pos[0] = 0;
            Pos++;
        }
        FileTypeStr[FileTypesLen] = 0;
        FileTypeStr[FileTypesLen + 1] = 0;
    }

    OPENFILENAME ofn;
    FMemory::Memzero(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = (HWND)ParentWindowHandle;
    ofn.lpstrFilter = FileTypesPtr;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = Filename;
    ofn.nMaxFile = MAX_FILENAME_STR;
    ofn.lpstrInitialDir = Pathname;
    ofn.lpstrTitle = *DialogTitle;
    if (FileTypesLen > 0) ofn.lpstrDefExt = &FileTypeStr[0];
    ofn.Flags = OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_EXPLORER;

    if (bSave)
    {
        ofn.Flags |= OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_NOVALIDATE;
    }
    else
    {
        ofn.Flags |= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    }

    if (Flags & EEasyFileDialogFlags::Multiple) ofn.Flags |= OFN_ALLOWMULTISELECT;

    bool bSuccess = bSave ? !!::GetSaveFileName(&ofn) : !!::GetOpenFileName(&ofn);

    if (bSuccess)
    {
        if (Flags & EEasyFileDialogFlags::Multiple)
        {
            FString DirectoryOrSingleFileName = FString(Filename);
            TCHAR* Pos = Filename + DirectoryOrSingleFileName.Len() + 1;
            if (Pos[0] == 0)
            {
                OutFilenames.Add(DirectoryOrSingleFileName);
            }
            else
            {
                FString SelectedFile;
                do
                {
                    SelectedFile = FString(Pos);
                    OutFilenames.Add(DirectoryOrSingleFileName / SelectedFile);
                    Pos += SelectedFile.Len() + 1;
                } while (Pos[0] != 0);
            }
        }
        else
        {
            OutFilenames.Add(FString(Filename));
        }

        OutFilterIndex = ofn.nFilterIndex - 1;
        FString Extension = CleanExtensionList.IsValidIndex(OutFilterIndex) ? CleanExtensionList[OutFilterIndex] : TEXT("");

        for (FString& OutFilename : OutFilenames)
        {
            OutFilename = IFileManager::Get().ConvertToRelativePath(*OutFilename);
            if (FPaths::GetExtension(OutFilename).IsEmpty() && !Extension.IsEmpty())
            {
                OutFilename += Extension;
            }
            FPaths::NormalizeFilename(OutFilename);
        }
    }
    else
    {
        uint32 Error = ::CommDlgExtendedError();
        if (Error != ERROR_SUCCESS)
        {
            // Log error if needed
        }
    }
    return bSuccess;
#endif
#pragma endregion

#pragma region Linux
#if PLATFORM_LINUX
    // Use Zenity for file dialogs on Linux
    FString Command = TEXT("zenity --file-selection");
    TArray<FString> UnformattedExtensions; // Declare here
    TArray<FString> CleanExtensionList;   // Declare here

    // Parse FileTypes into UnformattedExtensions and build CleanExtensionList
    FileTypes.ParseIntoArray(UnformattedExtensions, TEXT("|"), true);
    for (int32 i = 1; i < UnformattedExtensions.Num(); i += 2)
    {
        FString Ext = UnformattedExtensions[i];
        if (Ext != TEXT("*.*"))
        {
            int32 WildCardIndex = Ext.Find(TEXT("*"));
            CleanExtensionList.Add(WildCardIndex != INDEX_NONE ? Ext.RightChop(WildCardIndex + 1) : Ext);
        }
    }

    if (bSave)
    {
        Command += TEXT(" --save --confirm-overwrite");
    }
    else
    {
        Command += TEXT(" --file-filter='");
        // Convert FileTypes to Zenity filter format (e.g., "*.txt *.png")
        FString Filter;
        for (int32 i = 1; i < UnformattedExtensions.Num(); i += 2)
        {
            FString Ext = UnformattedExtensions[i];
            if (Ext != TEXT("*.*"))
            {
                Filter += Ext + TEXT(" ");
            }
        }
        Command += Filter.TrimEnd() + TEXT("'");
        if (Flags & EEasyFileDialogFlags::Multiple)
        {
            Command += TEXT(" --multiple");
        }
    }

    if (!DialogTitle.IsEmpty())
    {
        Command += TEXT(" --title='") + DialogTitle + TEXT("'");
    }

    if (!DefaultPath.IsEmpty())
    {
        FString FullPath = FPaths::ConvertRelativePathToFull(DefaultPath);
        Command += TEXT(" --filename='") + FullPath + TEXT("'");
    }

    if (!DefaultFile.IsEmpty() && !bSave)
    {
        Command += TEXT("/") + DefaultFile + TEXT("'");
    }

    // Execute Zenity and capture output
    FString Result, Errors;
    int32 ReturnCode;
    bool bSuccess = FPlatformProcess::ExecProcess(TEXT("/bin/sh"), *(TEXT("-c \"") + Command + TEXT("\"")), &ReturnCode, &Result, &Errors);

    if (bSuccess && ReturnCode == 0 && !Result.IsEmpty())
    {
        // Zenity returns paths separated by | for multiple selection
        if (Flags & EEasyFileDialogFlags::Multiple)
        {
            TArray<FString> SelectedFiles;
            Result.ParseIntoArray(SelectedFiles, TEXT("|"), true);
            for (const FString& File : SelectedFiles)
            {
                FString NormalizedFile = File;
                FPaths::NormalizeFilename(NormalizedFile);
                OutFilenames.Add(NormalizedFile);
            }
        }
        else
        {
            FString NormalizedFile = Result;
            FPaths::NormalizeFilename(NormalizedFile);
            OutFilenames.Add(NormalizedFile);
        }

        // Set OutFilterIndex (simplified for Linux; assumes first filter)
        OutFilterIndex = 0;

        // Normalize paths and add extensions if needed
        FString Extension = CleanExtensionList.IsValidIndex(OutFilterIndex) ? CleanExtensionList[OutFilterIndex] : TEXT("");

        for (FString& OutFilename : OutFilenames)
        {
            if (FPaths::GetExtension(OutFilename).IsEmpty() && !Extension.IsEmpty() && bSave)
            {
                OutFilename += TEXT(".") + Extension;
            }
            FPaths::NormalizeFilename(OutFilename);
        }
        return true;
    }
    return false;
#endif
#pragma endregion
    return false;
}

bool EFDCore::OpenFolderDialogInner(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName)
{
#pragma region Windows
#if PLATFORM_WINDOWS
    bool bSuccess = false;
    TComPtr<IFileOpenDialog> FileDialog;
    if (SUCCEEDED(::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&FileDialog))))
    {
        DWORD dwFlags = 0;
        FileDialog->GetOptions(&dwFlags);
        FileDialog->SetOptions(dwFlags | FOS_PICKFOLDERS);
        FileDialog->SetTitle(*DialogTitle);
        if (!DefaultPath.IsEmpty())
        {
            FString DefaultWindowsPath = FPaths::ConvertRelativePathToFull(DefaultPath);
            DefaultWindowsPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::CaseSensitive);
            TComPtr<IShellItem> DefaultPathItem;
            if (SUCCEEDED(::SHCreateItemFromParsingName(*DefaultWindowsPath, nullptr, IID_PPV_ARGS(&DefaultPathItem))))
            {
                FileDialog->SetFolder(DefaultPathItem);
            }
        }
        if (SUCCEEDED(FileDialog->Show((HWND)ParentWindowHandle)))
        {
            TComPtr<IShellItem> Result;
            if (SUCCEEDED(FileDialog->GetResult(&Result)))
            {
                PWSTR pFilePath = nullptr;
                if (SUCCEEDED(Result->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath)))
                {
                    bSuccess = true;
                    OutFolderName = pFilePath;
                    FPaths::NormalizeDirectoryName(OutFolderName);
                    ::CoTaskMemFree(pFilePath);
                }
            }
        }
    }
    return bSuccess;
#endif
#pragma endregion

#pragma region Linux
#if PLATFORM_LINUX
    // Use Zenity for folder selection
    FString Command = TEXT("zenity --file-selection --directory");
    if (!DialogTitle.IsEmpty())
    {
        Command += TEXT(" --title='") + DialogTitle + TEXT("'");
    }
    if (!DefaultPath.IsEmpty())
    {
        FString FullPath = FPaths::ConvertRelativePathToFull(DefaultPath);
        Command += TEXT(" --filename='") + FullPath + TEXT("'");
    }

    // Execute Zenity and capture output
    FString Result, Errors;
    int32 ReturnCode;
    bool bSuccess = FPlatformProcess::ExecProcess(TEXT("/bin/sh"), *(TEXT("-c \"") + Command + TEXT("\"")), &ReturnCode, &Result, &Errors);

    if (bSuccess && ReturnCode == 0 && !Result.IsEmpty())
    {
        OutFolderName = Result;
        FPaths::NormalizeDirectoryName(OutFolderName);
        return true;
    }
    return false;
#endif
#pragma endregion
    return false;
}