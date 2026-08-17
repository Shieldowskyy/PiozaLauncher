// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "FileNodes.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceDebug.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "GenericPlatform/GenericPlatformFile.h"
#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include <jni.h>
#endif

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_ANDROID || PLATFORM_IOS
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#endif

// ---------------------------------------------------------------------------
// Symlink detection / resolution
//
// NOTE: This is implemented with raw platform calls rather than a single
// IPlatformFile API because symlink support/queries are not uniformly exposed
// across every engine version's IPlatformFile interface. If your engine version
// does expose an equivalent (e.g. IPlatformFile::IsSymlink on newer UE5), feel
// free to swap it in here - just keep the "false/not-a-symlink on any doubt"
// fallback behavior.
// ---------------------------------------------------------------------------

bool UFileNodes::IsSymlinkPath(const FString& Path)
{
    #if PLATFORM_WINDOWS
    const DWORD Attributes = GetFileAttributesW(*Path);
    if (Attributes == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    return (Attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    #elif PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_ANDROID || PLATFORM_IOS
    struct stat StatBuf;
    if (lstat(TCHAR_TO_UTF8(*Path), &StatBuf) != 0)
    {
        return false;
    }
    return S_ISLNK(StatBuf.st_mode);
    #else
    // Unknown platform: assume "not a symlink" rather than silently following one.
    return false;
    #endif
}

// ---------------------------------------------------------------------------
// Protected-path safety net
// ---------------------------------------------------------------------------

static const TArray<FString>& GetProtectedRoots()
{
    static const TArray<FString> Roots = {
        #if PLATFORM_WINDOWS
        TEXT("C:/Windows"),
        TEXT("C:/Program Files"),
        TEXT("C:/Program Files (x86)"),
        TEXT("C:/ProgramData"),
        TEXT("C:/Users"),
        #else
        TEXT("/"),
        TEXT("/home"),
        TEXT("/root"),
        TEXT("/etc"),
        TEXT("/usr"),
        TEXT("/bin"),
        TEXT("/sbin"),
        TEXT("/lib"),
        TEXT("/lib64"),
        TEXT("/var"),
        TEXT("/opt"),
        TEXT("/boot"),
        TEXT("/dev"),
        TEXT("/proc"),
        TEXT("/sys"),
        TEXT("/System"),
        TEXT("/Library"),
        TEXT("/Applications"),
        TEXT("/Users"),
        #endif
    };
    return Roots;
}

bool UFileNodes::IsProtectedPath(const FString& InPath, FString& OutReason)
{
    if (InPath.IsEmpty())
    {
        OutReason = TEXT("Path is empty.");
        return true;
    }

    // Resolve to an absolute, normalized path so relative paths / "../" tricks and
    // trailing slashes can't slip past the checks below.
    FString FullPath = FPaths::ConvertRelativePathToFull(InPath);
    FPaths::NormalizeDirectoryName(FullPath);

    // If the path itself is a symlink, resolve it to what it actually points at
    // before checking - otherwise a link named e.g. "TempCache" that happens to
    // point at /home could sail straight through the checks below.
    #if PLATFORM_WINDOWS
    if (IsSymlinkPath(FullPath))
    {
        HANDLE Handle = CreateFileW(*FullPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (Handle != INVALID_HANDLE_VALUE)
        {
            WCHAR Resolved[MAX_PATH];
            if (GetFinalPathNameByHandleW(Handle, Resolved, MAX_PATH, FILE_NAME_NORMALIZED) > 0)
            {
                FullPath = FString(Resolved);
                FullPath.ReplaceInline(TEXT("\\\\?\\"), TEXT(""));
            }
            CloseHandle(Handle);
        }
    }
    #elif PLATFORM_LINUX || PLATFORM_MAC
    if (IsSymlinkPath(FullPath))
    {
        char Resolved[PATH_MAX];
        if (realpath(TCHAR_TO_UTF8(*FullPath), Resolved) != nullptr)
        {
            FullPath = UTF8_TO_TCHAR(Resolved);
        }
    }
    #endif

    FullPath.ReplaceInline(TEXT("\\"), TEXT("/"));
    while (FullPath.Len() > 1 && FullPath.EndsWith(TEXT("/")))
    {
        FullPath.LeftChopInline(1);
    }

    #if PLATFORM_WINDOWS
    const ESearchCase::Type SearchCase = ESearchCase::IgnoreCase;
    #else
    const ESearchCase::Type SearchCase = ESearchCase::CaseSensitive;
    #endif

    // 1) Explicit deny-list of well-known critical system locations.
    for (const FString& Root : GetProtectedRoots())
    {
        if (FullPath.Equals(Root, SearchCase))
        {
            OutReason = FString::Printf(TEXT("'%s' is a protected system directory."), *FullPath);
            return true;
        }
    }

    // 2) The current user's real home/profile directory, wherever it actually lives.
    const FString HomeDir = FPlatformProcess::UserHomeDir();
    if (!HomeDir.IsEmpty())
    {
        FString NormalizedHome = HomeDir;
        NormalizedHome.ReplaceInline(TEXT("\\"), TEXT("/"));
        while (NormalizedHome.Len() > 1 && NormalizedHome.EndsWith(TEXT("/")))
        {
            NormalizedHome.LeftChopInline(1);
        }
        if (FullPath.Equals(NormalizedHome, SearchCase))
        {
            OutReason = FString::Printf(TEXT("'%s' is the current user's home directory."), *FullPath);
            return true;
        }
    }

    // 3) Generic pattern match for "/home/<user>" and "<drive>:/Users/<user>", so we're
    //    not solely relying on FPlatformProcess/the exact username being known.
    if (FullPath.StartsWith(TEXT("/home/")))
    {
        const FString After = FullPath.RightChop(6); // strip "/home/"
        if (!After.IsEmpty() && !After.Contains(TEXT("/")))
        {
            OutReason = FString::Printf(TEXT("'%s' looks like a user's home directory."), *FullPath);
            return true;
        }
    }
    if (FullPath.MatchesWildcard(TEXT("?:/Users/*"), ESearchCase::IgnoreCase))
    {
        const int32 UsersIdx = FullPath.Find(TEXT("/Users/"));
        const FString After = FullPath.RightChop(UsersIdx + 7);
        if (!After.IsEmpty() && !After.Contains(TEXT("/")))
        {
            OutReason = FString::Printf(TEXT("'%s' looks like a user's profile directory."), *FullPath);
            return true;
        }
    }

    // 4) Refuse drive roots outright (C:/, D:/, ...).
    if (FullPath.Len() <= 3 && FullPath.Len() >= 2 && FullPath.Mid(1, 1) == TEXT(":"))
    {
        OutReason = FString::Printf(TEXT("'%s' is a drive root."), *FullPath);
        return true;
    }

    // 5) Defense in depth: refuse anything sitting at the very top of a filesystem
    //    root (depth below MinAllowedPathDepth) even if it isn't on the explicit list
    //    above - this catches things like "/mnt", "/media", "D:/Users" etc. without
    //    having to name every possible mount point. Tune MinAllowedPathDepth if this
    //    is too conservative for your project's needs.
    constexpr int32 MinAllowedPathDepth = 2;
    TArray<FString> Segments;
    FullPath.ParseIntoArray(Segments, TEXT("/"), true);
    #if PLATFORM_WINDOWS
    // The drive letter ("C:") is the root here, not a real path segment.
    if (Segments.Num() > 0 && Segments[0].EndsWith(TEXT(":")))
    {
        Segments.RemoveAt(0);
    }
    #endif
    if (Segments.Num() < MinAllowedPathDepth)
    {
        OutReason = FString::Printf(TEXT("'%s' is too close to the filesystem root to operate on safely."), *FullPath);
        return true;
    }

    return false;
}

FString UFileNodes::ResolveFullDirectoryPath(const FString& Path)
{
    FString FullPath = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeDirectoryName(FullPath);
    return FullPath;
}

FString UFileNodes::ResolveFullFilePath(const FString& Path)
{
    FString FullPath = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(FullPath);
    return FullPath;
}

// ---------------------------------------------------------------------------
// Existing nodes
// ---------------------------------------------------------------------------

bool UFileNodes::ReadText(const FString& FilePath, FString& OutText)
{
    // Avoid FFileHelper errors in the log if the file doesn't exist
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        return false;
    }
    return FFileHelper::LoadFileToString(OutText, *FilePath);
}

bool UFileNodes::ReadBytes(const FString& FilePath, TArray<uint8>& OutBytes)
{
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        return false;
    }
    return FFileHelper::LoadFileToArray(OutBytes, *FilePath);
}

int64 UFileNodes::GetFileSize(const FString& FilePath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.FileSize(*FilePath);
}

bool UFileNodes::SaveText(const FString& FilePath, const FString& Text, bool bAppend, bool bForceOverwrite, ETextEncodingFormat Encoding, FString& OutError)
{
    IFileManager& FileManager = IFileManager::Get();
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    const FString FullPath = ResolveFullFilePath(FilePath);

    // Handle read-only files if overwrite is forced
    if (bForceOverwrite && !bAppend)
    {
        if (PlatformFile.FileExists(*FullPath))
        {
            // Force-overwriting an existing file is effectively delete+write, so it
            // gets the same protected-path safety net as DeleteFile/DeleteDirectory.
            FString ProtectReason;
            if (IsProtectedPath(FullPath, ProtectReason))
            {
                OutError = FString::Printf(TEXT("Refusing to force-overwrite '%s': %s"), *FullPath, *ProtectReason);
                UE_LOG(LogTemp, Error, TEXT("SaveText blocked: %s"), *OutError);
                return false;
            }

            if (PlatformFile.IsReadOnly(*FullPath))
            {
                if (!PlatformFile.SetReadOnly(*FullPath, false))
                {
                    OutError = FString::Printf(TEXT("Failed to remove ReadOnly flag: %s"), *FullPath);
                    return false;
                }
            }
        }
    }

    const uint32 WriteFlags = (bAppend ? FILEWRITE_Append : FILEWRITE_None);

    // Special handling for UTF-8 without BOM (FFileHelper defaults to BOM for ForceUTF8)
    if (Encoding == ETextEncodingFormat::UTF8WithoutBOM)
    {
        FTCHARToUTF8 UTF8Converter(*Text);
        TArray<uint8> UTF8Data;
        UTF8Data.Append(reinterpret_cast<const uint8*>(UTF8Converter.Get()), UTF8Converter.Length());

        if (!FFileHelper::SaveArrayToFile(UTF8Data, *FullPath, &FileManager, WriteFlags))
        {
            OutError = FString::Printf(TEXT("Failed to write UTF-8 (no BOM) file: %s"), *FullPath);
            return false;
        }
        return true;
    }

    FFileHelper::EEncodingOptions ChosenEncoding;
    switch (Encoding)
    {
        case ETextEncodingFormat::ANSI:
            ChosenEncoding = FFileHelper::EEncodingOptions::ForceAnsi;
            break;
        case ETextEncodingFormat::UTF8:
            ChosenEncoding = FFileHelper::EEncodingOptions::ForceUTF8;
            break;
        case ETextEncodingFormat::UTF16:
            ChosenEncoding = FFileHelper::EEncodingOptions::ForceUnicode;
            break;
        case ETextEncodingFormat::AutoDetect:
        default:
            ChosenEncoding = FFileHelper::EEncodingOptions::AutoDetect;
            break;
    }

    if (!FFileHelper::SaveStringToFile(Text, *FullPath, ChosenEncoding, &FileManager, WriteFlags))
    {
        OutError = FString::Printf(TEXT("Failed to write to file: %s"), *FullPath);
        return false;
    }

    return true;
}

bool UFileNodes::ListDirectory(const FString& DirPath, const FString& Pattern, bool bShowFiles, bool bShowDirectories, bool bRecursive, TArray<FString>& OutNodes)
{
    IFileManager& FileManager = IFileManager::Get();
    const FString FinalPattern = Pattern.IsEmpty() ? TEXT("*") : Pattern;
    TArray<FString> Results;

    if (bRecursive)
    {
        FileManager.FindFilesRecursive(Results, *DirPath, *FinalPattern, bShowFiles, bShowDirectories);
    }
    else
    {
        if (bShowFiles)
        {
            FileManager.FindFiles(Results, *DirPath, *FinalPattern);
        }

        if (bShowDirectories)
        {
            TArray<FString> DirResults;
            // FindFiles works differently for directories, strictly requiring the wildcard
            FileManager.FindFiles(DirResults, *(DirPath / TEXT("*")), /* Files = */ false, /* Directories = */ true);
            Results.Append(DirResults);
        }
    }

    if (Results.Num() == 0)
    {
        return false;
    }

    OutNodes = Results;
    return true;
}

bool UFileNodes::BrowseDirectory(const FString& DirectoryPath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    FString NormalizedPath = FPaths::ConvertRelativePathToFull(DirectoryPath);
    FPaths::NormalizeDirectoryName(NormalizedPath);

    if (!PlatformFile.DirectoryExists(*NormalizedPath))
    {
        UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Path is not a valid directory: %s"), *NormalizedPath);
        return false;
    }

    // NormalizedPath gets wrapped in quotes below to build a command-line string for
    // explorer.exe/xdg-open. If the path itself contained a `"`, that would let it
    // break out of the quoted argument and inject extra command-line arguments -
    // refuse rather than risk that.
    if (NormalizedPath.Contains(TEXT("\"")))
    {
        UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Path contains a quote character and was rejected: %s"), *NormalizedPath);
        return false;
    }

    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        // Android API 24+ throws FileUriExposedException if we try to share a "file://" URI.
        // To fix this without complex FileProvider setup in manifest,
        // we temporarily disable the StrictMode VM policy for file URIs.

        // 1. Get StrictMode class and methods
        jclass StrictModeClass = Env->FindClass("android/os/StrictMode");
        jmethodID DisableDeathMethod = Env->GetStaticMethodID(StrictModeClass, "disableDeathOnFileUriExposure", "()V");

        if (DisableDeathMethod)
        {
            Env->CallStaticVoidMethod(StrictModeClass, DisableDeathMethod);
        }

        // 2. Prepare the URI (Must use file:// protocol)
        FString UriString = FString(TEXT("file://")) + NormalizedPath;
        jstring jUriString = Env->NewStringUTF(TCHAR_TO_UTF8(*UriString));

        jclass UriClass = Env->FindClass("android/net/Uri");
        jmethodID ParseMethod = Env->GetStaticMethodID(UriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
        jobject UriObject = Env->CallStaticObjectMethod(UriClass, ParseMethod, jUriString);

        // 3. Create Intent (ACTION_VIEW)
        jclass IntentClass = Env->FindClass("android/content/Intent");
        jstring ActionView = Env->NewStringUTF("android.intent.action.VIEW");
        jmethodID IntentCtor = Env->GetMethodID(IntentClass, "<init>", "(Ljava/lang/String;)V");
        jobject IntentObject = Env->NewObject(IntentClass, IntentCtor, ActionView);

        // 4. Set Data and Type
        // "resource/folder" is a de-facto standard for file managers.
        // "vnd.android.cursor.dir/file" is another option, but resource/folder is widely supported by 3rd party apps.
        jmethodID SetDataAndTypeMethod = Env->GetMethodID(IntentClass, "setDataAndType", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;");
        jstring MimeType = Env->NewStringUTF("resource/folder");
        Env->CallObjectMethod(IntentObject, SetDataAndTypeMethod, UriObject, MimeType);

        // 5. Add Flags
        // FLAG_ACTIVITY_NEW_TASK (0x10000000) is required when starting activity from non-activity context
        // FLAG_GRANT_READ_URI_PERMISSION (0x00000001) is good practice
        jmethodID AddFlagsMethod = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
        Env->CallObjectMethod(IntentObject, AddFlagsMethod, 0x10000000 | 0x00000001);

        // 6. Start Activity safely
        jobject GameActivity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(GameActivity);
        jmethodID StartActivityMethod = Env->GetMethodID(ActivityClass, "startActivity", "(Landroid/content/Intent;)V");

        // We wrap this in a try-catch equivalent (checking for ActivityNotFoundException)
        // However, in JNI C++, we should check if the intent resolves to avoid a crash,
        // or let the Java side handle the exception.
        // Here we attempt to start it. If no file manager is installed, this might throw a Java exception.
        // Ideally, you would check resolveActivity in Java, but for brevity:

        bool bExceptionOccurred = false;
        Env->CallVoidMethod(GameActivity, StartActivityMethod, IntentObject);

        if (Env->ExceptionCheck())
        {
            Env->ExceptionDescribe(); // Log exception to Logcat
            Env->ExceptionClear();    // Clear it so app doesn't crash
            UE_LOG(LogTemp, Warning, TEXT("BrowseDirectory: No application found to handle directory browsing."));

            // Fallback: Try generic generic mime type if resource/folder failed
            jstring WildcardMime = Env->NewStringUTF("*/*");
            Env->CallObjectMethod(IntentObject, SetDataAndTypeMethod, UriObject, WildcardMime);
            Env->CallVoidMethod(GameActivity, StartActivityMethod, IntentObject);

            if (Env->ExceptionCheck())
            {
                Env->ExceptionClear();
                UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Failed to open directory even with wildcard fallback."));
                return false;
            }
        }

        UE_LOG(LogTemp, Log, TEXT("BrowseDirectory: Android Intent sent for path: %s"), *UriString);
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Failed to get JNI Environment"));
    return false;

    #elif PLATFORM_WINDOWS
    // Windows Explorer handles backslashes better in some edge cases
    NormalizedPath.ReplaceInline(TEXT("/"), TEXT("\\"), ESearchCase::IgnoreCase);

    const FString Command = TEXT("explorer.exe");
    // Quotes are required to handle paths with spaces
    const FString Params = FString::Printf(TEXT("\"%s\""), *NormalizedPath);

    FPlatformProcess::CreateProc(*Command, *Params, true, false, false, nullptr, 0, nullptr, nullptr);
    return true;

    #elif PLATFORM_LINUX
    const FString Command = TEXT("xdg-open");
    const FString Params = FString::Printf(TEXT("\"%s\""), *NormalizedPath);

    FPlatformProcess::CreateProc(*Command, *Params, true, false, false, nullptr, 0, nullptr, nullptr);
    return true;

    #else
    UE_LOG(LogTemp, Error, TEXT("BrowseDirectory: Unsupported platform"));
    return false;
    #endif
}

// ---------------------------------------------------------------------------
// DeleteFile
// ---------------------------------------------------------------------------

bool UFileNodes::DeleteFile(const FString& FilePath, FString& OutError)
{
    if (FilePath.IsEmpty())
    {
        OutError = TEXT("FilePath is empty.");
        return false;
    }

    const FString FullPath = ResolveFullFilePath(FilePath);

    FString ProtectReason;
    if (IsProtectedPath(FullPath, ProtectReason))
    {
        OutError = FString::Printf(TEXT("Refusing to delete '%s': %s"), *FullPath, *ProtectReason);
        UE_LOG(LogTemp, Error, TEXT("DeleteFile blocked: %s"), *OutError);
        return false;
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    if (PlatformFile.DirectoryExists(*FullPath))
    {
        OutError = FString::Printf(TEXT("'%s' is a directory, not a file - use DeleteDirectory instead."), *FullPath);
        return false;
    }

    if (!PlatformFile.FileExists(*FullPath))
    {
        // Nothing to do - treat as success, matching common "delete if exists" semantics.
        return true;
    }

    if (PlatformFile.IsReadOnly(*FullPath))
    {
        PlatformFile.SetReadOnly(*FullPath, false);
    }

    if (!PlatformFile.DeleteFile(*FullPath))
    {
        OutError = FString::Printf(TEXT("Failed to delete file: %s"), *FullPath);
        UE_LOG(LogTemp, Error, TEXT("DeleteFile failed: %s"), *OutError);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// DeleteDirectory
// ---------------------------------------------------------------------------

bool UFileNodes::DeleteDirectoryLinkOnly(const FString& FullPath, FString& OutError)
{
    #if PLATFORM_WINDOWS
    if (!::RemoveDirectoryW(*FullPath))
    {
        OutError = FString::Printf(TEXT("Failed to remove symlink: %s"), *FullPath);
        return false;
    }
    #elif PLATFORM_LINUX || PLATFORM_MAC || PLATFORM_ANDROID || PLATFORM_IOS
    if (unlink(TCHAR_TO_UTF8(*FullPath)) != 0)
    {
        OutError = FString::Printf(TEXT("Failed to remove symlink: %s"), *FullPath);
        return false;
    }
    #else
    OutError = TEXT("Symlink removal not implemented for this platform.");
    return false;
    #endif
    return true;
}

bool UFileNodes::DeleteDirectoryInternal(const FString& FullPath, bool bFollowSymlinks, FString& OutError, int32 Depth)
{
    constexpr int32 MaxRecursionDepth = 64;
    if (Depth > MaxRecursionDepth)
    {
        OutError = TEXT("Maximum recursion depth exceeded (possible symlink loop).");
        return false;
    }

    // Never walk into a symlinked directory unless explicitly asked to - just remove
    // the link itself and stop, so a link that points somewhere unexpected (like the
    // user's home folder) can't cause us to recurse into and wipe it out.
    if (IsSymlinkPath(FullPath) && !bFollowSymlinks)
    {
        return DeleteDirectoryLinkOnly(FullPath, OutError);
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    IFileManager& FileManager = IFileManager::Get();

    TArray<FString> ChildFiles;
    TArray<FString> ChildDirs;
    FileManager.FindFiles(ChildFiles, *(FullPath / TEXT("*")), /* Files = */ true, /* Directories = */ false);
    FileManager.FindFiles(ChildDirs, *(FullPath / TEXT("*")), /* Files = */ false, /* Directories = */ true);

    for (const FString& FileName : ChildFiles)
    {
        const FString ChildPath = FullPath / FileName;
        if (PlatformFile.IsReadOnly(*ChildPath))
        {
            PlatformFile.SetReadOnly(*ChildPath, false);
        }
        if (!PlatformFile.DeleteFile(*ChildPath))
        {
            OutError = FString::Printf(TEXT("Failed to delete file: %s"), *ChildPath);
            return false;
        }
    }

    for (const FString& DirName : ChildDirs)
    {
        const FString ChildPath = FullPath / DirName;
        if (!DeleteDirectoryInternal(ChildPath, bFollowSymlinks, OutError, Depth + 1))
        {
            return false;
        }
    }

    if (!PlatformFile.DeleteDirectory(*FullPath))
    {
        OutError = FString::Printf(TEXT("Failed to remove directory: %s"), *FullPath);
        return false;
    }

    return true;
}

bool UFileNodes::DeleteDirectory(const FString& DirectoryPath, bool bRecursive, bool bFollowSymlinks, FString& OutError)
{
    if (DirectoryPath.IsEmpty())
    {
        OutError = TEXT("DirectoryPath is empty.");
        return false;
    }

    const FString FullPath = ResolveFullDirectoryPath(DirectoryPath);

    FString ProtectReason;
    if (IsProtectedPath(FullPath, ProtectReason))
    {
        OutError = FString::Printf(TEXT("Refusing to delete '%s': %s"), *FullPath, *ProtectReason);
        UE_LOG(LogTemp, Error, TEXT("DeleteDirectory blocked: %s"), *OutError);
        return false;
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

    if (!PlatformFile.DirectoryExists(*FullPath))
    {
        // Nothing to do - matches "delete if exists" semantics used elsewhere in this library.
        return true;
    }

    if (!bRecursive)
    {
        if (!PlatformFile.DeleteDirectory(*FullPath))
        {
            OutError = FString::Printf(TEXT("Failed to delete directory (is it empty?): %s"), *FullPath);
            return false;
        }
        return true;
    }

    if (!DeleteDirectoryInternal(FullPath, bFollowSymlinks, OutError))
    {
        UE_LOG(LogTemp, Error, TEXT("DeleteDirectory failed: %s"), *OutError);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// CopyDirectory
// ---------------------------------------------------------------------------

bool UFileNodes::CopyDirectoryInternal(const FString& SourcePath, const FString& DestPath, bool bFollowSymlinks, bool bOverwriteExisting, FString& OutError, int32 Depth)
{
    constexpr int32 MaxRecursionDepth = 64;
    if (Depth > MaxRecursionDepth)
    {
        OutError = TEXT("Maximum recursion depth exceeded (possible symlink loop).");
        return false;
    }

    if (IsSymlinkPath(SourcePath) && !bFollowSymlinks)
    {
        // Skip symlinked subdirectories entirely rather than copying their contents.
        return true;
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    IFileManager& FileManager = IFileManager::Get();

    if (!PlatformFile.DirectoryExists(*DestPath))
    {
        if (!FileManager.MakeDirectory(*DestPath, /* Tree = */ true))
        {
            OutError = FString::Printf(TEXT("Failed to create directory: %s"), *DestPath);
            return false;
        }
    }

    TArray<FString> ChildFiles;
    TArray<FString> ChildDirs;
    FileManager.FindFiles(ChildFiles, *(SourcePath / TEXT("*")), /* Files = */ true, /* Directories = */ false);
    FileManager.FindFiles(ChildDirs, *(SourcePath / TEXT("*")), /* Files = */ false, /* Directories = */ true);

    for (const FString& FileName : ChildFiles)
    {
        const FString SrcFile = SourcePath / FileName;
        const FString DstFile = DestPath / FileName;

        if (IsSymlinkPath(SrcFile) && !bFollowSymlinks)
        {
            continue; // don't copy symlinked files through by default either
        }

        if (PlatformFile.FileExists(*DstFile) && !bOverwriteExisting)
        {
            continue;
        }

        if (!PlatformFile.CopyFile(*DstFile, *SrcFile))
        {
            OutError = FString::Printf(TEXT("Failed to copy file: %s"), *SrcFile);
            return false;
        }
    }

    for (const FString& DirName : ChildDirs)
    {
        if (!CopyDirectoryInternal(SourcePath / DirName, DestPath / DirName, bFollowSymlinks, bOverwriteExisting, OutError, Depth + 1))
        {
            return false;
        }
    }

    return true;
}

bool UFileNodes::CopyDirectory(const FString& SourceDir, const FString& DestDir, bool bFollowSymlinks, bool bOverwriteExisting, FString& OutError)
{
    if (SourceDir.IsEmpty() || DestDir.IsEmpty())
    {
        OutError = TEXT("SourceDir/DestDir is empty.");
        return false;
    }

    const FString FullSource = ResolveFullDirectoryPath(SourceDir);
    const FString FullDest = ResolveFullDirectoryPath(DestDir);

    FString ProtectReason;
    if (IsProtectedPath(FullDest, ProtectReason))
    {
        OutError = FString::Printf(TEXT("Refusing to copy into '%s': %s"), *FullDest, *ProtectReason);
        UE_LOG(LogTemp, Error, TEXT("CopyDirectory blocked: %s"), *OutError);
        return false;
    }

    // Copying a directory tree into itself (or into a subfolder of itself) would
    // recurse forever / corrupt data - refuse outright.
    if (FullDest.Equals(FullSource, ESearchCase::IgnoreCase) ||
        FullDest.StartsWith(FullSource + TEXT("/"), ESearchCase::IgnoreCase))
    {
        OutError = TEXT("Destination is the same as, or nested inside, the source directory.");
        return false;
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*FullSource))
    {
        OutError = FString::Printf(TEXT("Source directory does not exist: %s"), *FullSource);
        return false;
    }

    return CopyDirectoryInternal(FullSource, FullDest, bFollowSymlinks, bOverwriteExisting, OutError, 0);
}
