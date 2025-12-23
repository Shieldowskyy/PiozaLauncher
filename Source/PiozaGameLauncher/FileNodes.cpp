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

bool UFileNodes::CopyDirectory(const FString& SourceDir, const FString& DestDir)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    return PlatformFile.CopyDirectoryTree(*DestDir, *SourceDir, /* bOverwriteExisting = */ true);
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

    // Handle read-only files if overwrite is forced
    if (bForceOverwrite && !bAppend)
    {
        if (PlatformFile.FileExists(*FilePath))
        {
            if (PlatformFile.IsReadOnly(*FilePath))
            {
                if (!PlatformFile.SetReadOnly(*FilePath, false))
                {
                    OutError = FString::Printf(TEXT("Failed to remove ReadOnly flag: %s"), *FilePath);
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

        if (!FFileHelper::SaveArrayToFile(UTF8Data, *FilePath, &FileManager, WriteFlags))
        {
            OutError = FString::Printf(TEXT("Failed to write UTF-8 (no BOM) file: %s"), *FilePath);
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

    if (!FFileHelper::SaveStringToFile(Text, *FilePath, ChosenEncoding, &FileManager, WriteFlags))
    {
        OutError = FString::Printf(TEXT("Failed to write to file: %s"), *FilePath);
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