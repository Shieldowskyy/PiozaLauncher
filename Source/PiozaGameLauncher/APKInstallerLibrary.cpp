// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "APKInstallerLibrary.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#endif

EAPKInstallError UAPKInstallerLibrary::InstallAPK(const FString& APKFilePath, FString& OutErrorMessage)
{
    OutErrorMessage = TEXT("");

#if PLATFORM_ANDROID
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    if (!Env)
    {
        OutErrorMessage = TEXT("Failed to get JNI environment");
        return EAPKInstallError::NoJNIEnvironment;
    }

    jobject Activity = FAndroidApplication::GetGameActivityThis();
    if (!Activity)
    {
        OutErrorMessage = TEXT("Failed to get game activity");
        return EAPKInstallError::NoJNIEnvironment;
    }

    // Get Activity class and ClassLoader
    jclass ActivityClass = Env->GetObjectClass(Activity);
    jmethodID GetClassLoader = Env->GetMethodID(ActivityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject Loader = Env->CallObjectMethod(Activity, GetClassLoader);

    // Load FileProvider via Class.forName with class loader
    jclass ClassClass = Env->FindClass("java/lang/Class");
    jmethodID ForNameMethod = Env->GetStaticMethodID(ClassClass, "forName",
                                                     "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
    jstring FileProviderName = Env->NewStringUTF("androidx.core.content.FileProvider");
    jclass FileProviderClass = (jclass)Env->CallStaticObjectMethod(ClassClass, ForNameMethod,
                                                                   FileProviderName, JNI_TRUE, Loader);

    Env->DeleteLocalRef(FileProviderName);
    Env->DeleteLocalRef(ClassClass);

    if (!FileProviderClass)
    {
        OutErrorMessage = TEXT("Failed to find FileProvider class with ClassLoader");
        return EAPKInstallError::FileProviderClassNotFound;
    }

    // Prepare APK file
    jstring JFilePath = Env->NewStringUTF(TCHAR_TO_UTF8(*APKFilePath));
    jclass FileClass = Env->FindClass("java/io/File");
    jmethodID FileConstructor = Env->GetMethodID(FileClass, "<init>", "(Ljava/lang/String;)V");
    jobject FileObject = Env->NewObject(FileClass, FileConstructor, JFilePath);

    jmethodID ExistsMethod = Env->GetMethodID(FileClass, "exists", "()Z");
    if (!Env->CallBooleanMethod(FileObject, ExistsMethod))
    {
        OutErrorMessage = FString::Printf(TEXT("APK file does not exist: %s"), *APKFilePath);
        return EAPKInstallError::FileNotFound;
    }

    // Get application context
    jmethodID GetApplicationContext = Env->GetMethodID(ActivityClass, "getApplicationContext", "()Landroid/content/Context;");
    jobject Context = Env->CallObjectMethod(Activity, GetApplicationContext);

    // Authority from manifest (hardcoded to match manifest)
    jstring JAuthority = Env->NewStringUTF("yt.dsh.PiozaGameLauncher.fileprovider");

    // Get URI
    jmethodID GetUriForFile = Env->GetStaticMethodID(FileProviderClass, "getUriForFile",
                                                     "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;");
    jobject Uri = Env->CallStaticObjectMethod(FileProviderClass, GetUriForFile, Context, JAuthority, FileObject);

    if (!Uri)
    {
        OutErrorMessage = TEXT("Failed to get URI from FileProvider. Check filepaths.xml and authority.");
        return EAPKInstallError::FileProviderURICreationFailed;
    }

    // Create Intent
    jclass IntentClass = Env->FindClass("android/content/Intent");
    jmethodID IntentConstructor = Env->GetMethodID(IntentClass, "<init>", "(Ljava/lang/String;)V");
    jstring ActionInstall = Env->NewStringUTF("android.intent.action.INSTALL_PACKAGE");
    jobject Intent = Env->NewObject(IntentClass, IntentConstructor, ActionInstall);

    jmethodID SetDataAndType = Env->GetMethodID(IntentClass, "setDataAndType",
                                                "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;");
    jstring MimeType = Env->NewStringUTF("application/vnd.android.package-archive");
    Env->CallObjectMethod(Intent, SetDataAndType, Uri, MimeType);

    // Add flags
    jmethodID AddFlags = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
    Env->CallObjectMethod(Intent, AddFlags, 0x00000001); // FLAG_GRANT_READ_URI_PERMISSION
    Env->CallObjectMethod(Intent, AddFlags, 0x10000000); // FLAG_ACTIVITY_NEW_TASK

    // Start installation activity
    jmethodID StartActivity = Env->GetMethodID(ActivityClass, "startActivity", "(Landroid/content/Intent;)V");
    Env->CallVoidMethod(Activity, StartActivity, Intent);

    OutErrorMessage = TEXT("APK installation started successfully");
    return EAPKInstallError::Success;

#else
    OutErrorMessage = TEXT("InstallAPK only works on Android platform");
    return EAPKInstallError::NotAndroidPlatform;
#endif
}


bool UAPKInstallerLibrary::CanInstallFromUnknownSources()
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);

        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);

        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);
        jmethodID CanRequestPackageInstalls = Env->GetMethodID(PackageManagerClass, "canRequestPackageInstalls", "()Z");

        jboolean CanInstall = false;
        if (CanRequestPackageInstalls != nullptr)
        {
            CanInstall = Env->CallBooleanMethod(PackageManager, CanRequestPackageInstalls);
        }

        Env->DeleteLocalRef(ActivityClass);
        Env->DeleteLocalRef(PackageManager);
        Env->DeleteLocalRef(PackageManagerClass);

        return CanInstall;
    }
    #endif
    return false;
}

void UAPKInstallerLibrary::OpenInstallPermissionSettings()
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);

        // Create Intent for installation settings
        jclass IntentClass = Env->FindClass("android/content/Intent");
        jmethodID IntentConstructor = Env->GetMethodID(IntentClass, "<init>", "(Ljava/lang/String;)V");

        jstring ActionManageUnknownAppSources = Env->NewStringUTF("android.settings.MANAGE_UNKNOWN_APP_SOURCES");
        jobject Intent = Env->NewObject(IntentClass, IntentConstructor, ActionManageUnknownAppSources);

        // Add flag
        jmethodID AddFlags = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
        Env->CallObjectMethod(Intent, AddFlags, 0x10000000); // FLAG_ACTIVITY_NEW_TASK

        // Start activity
        jmethodID StartActivity = Env->GetMethodID(ActivityClass, "startActivity", "(Landroid/content/Intent;)V");
        Env->CallVoidMethod(Activity, StartActivity, Intent);

        // Cleanup
        Env->DeleteLocalRef(ActivityClass);
        Env->DeleteLocalRef(IntentClass);
        Env->DeleteLocalRef(Intent);
        Env->DeleteLocalRef(ActionManageUnknownAppSources);
    }
    #endif
}

FString UAPKInstallerLibrary::GetAppPackageName()
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        if (!Activity)
        {
            UE_LOG(LogTemp, Warning, TEXT("GetAppPackageName: Activity is null"));
            return TEXT("");
        }

        jclass ActivityClass = Env->GetObjectClass(Activity);
        if (!ActivityClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("GetAppPackageName: ActivityClass is null"));
            return TEXT("");
        }

        // getPackageName() -> String
        jmethodID GetPackageName = Env->GetMethodID(ActivityClass, "getPackageName", "()Ljava/lang/String;");
        if (!GetPackageName)
        {
            UE_LOG(LogTemp, Warning, TEXT("GetAppPackageName: getPackageName method not found"));
            Env->DeleteLocalRef(ActivityClass);
            return TEXT("");
        }

        jstring PackageName = (jstring)Env->CallObjectMethod(Activity, GetPackageName);
        FString Result;

        if (PackageName)
        {
            const char* PackageNameChars = Env->GetStringUTFChars(PackageName, nullptr);
            Result = UTF8_TO_TCHAR(PackageNameChars);
            Env->ReleaseStringUTFChars(PackageName, PackageNameChars);
            Env->DeleteLocalRef(PackageName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GetAppPackageName: PackageName is null"));
        }

        Env->DeleteLocalRef(ActivityClass);
        return Result;
    }
    #endif

    UE_LOG(LogTemp, Warning, TEXT("GetAppPackageName: Only works on Android"));
    return TEXT("");
}

bool UAPKInstallerLibrary::IsPackageInstalled(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);

        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);
        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);

        jmethodID GetPackageInfo = Env->GetMethodID(PackageManagerClass, "getPackageInfo",
                                                    "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
        jstring JPackageName = Env->NewStringUTF(TCHAR_TO_UTF8(*PackageName));

        bool bInstalled = true;
        jobject PackageInfo = Env->CallObjectMethod(PackageManager, GetPackageInfo, JPackageName, 0);

        if (Env->ExceptionCheck())
        {
            Env->ExceptionClear(); // Package not found
            bInstalled = false;
        }

        Env->DeleteLocalRef(JPackageName);
        if (PackageInfo) Env->DeleteLocalRef(PackageInfo);
        Env->DeleteLocalRef(PackageManagerClass);
        Env->DeleteLocalRef(PackageManager);
        Env->DeleteLocalRef(ActivityClass);

        return bInstalled;
    }
    #endif
    return false;
}

bool UAPKInstallerLibrary::LaunchApp(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);

        // Get PackageManager
        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);
        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);

        // getLaunchIntentForPackage
        jmethodID GetLaunchIntent = Env->GetMethodID(PackageManagerClass, "getLaunchIntentForPackage",
                                                     "(Ljava/lang/String;)Landroid/content/Intent;");
        jstring JPackageName = Env->NewStringUTF(TCHAR_TO_UTF8(*PackageName));

        jobject LaunchIntent = Env->CallObjectMethod(PackageManager, GetLaunchIntent, JPackageName);

        bool bLaunched = false;
        if (LaunchIntent != nullptr)
        {
            // Add FLAG_ACTIVITY_NEW_TASK
            jclass IntentClass = Env->GetObjectClass(LaunchIntent);
            jmethodID AddFlags = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
            Env->CallObjectMethod(LaunchIntent, AddFlags, 0x10000000);

            // Start activity
            jmethodID StartActivity = Env->GetMethodID(ActivityClass, "startActivity", "(Landroid/content/Intent;)V");
            Env->CallVoidMethod(Activity, StartActivity, LaunchIntent);

            Env->DeleteLocalRef(IntentClass);
            bLaunched = true;
        }

        Env->DeleteLocalRef(JPackageName);
        if (LaunchIntent) Env->DeleteLocalRef(LaunchIntent);
        Env->DeleteLocalRef(PackageManagerClass);
        Env->DeleteLocalRef(PackageManager);
        Env->DeleteLocalRef(ActivityClass);

        return bLaunched;
    }
    #endif
    return false;
}

void UAPKInstallerLibrary::UninstallPackage(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        // Retrieve the current activity
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);

        // Find the Intent class
        jclass IntentClass = Env->FindClass("android/content/Intent");
        jmethodID IntentConstructor = Env->GetMethodID(IntentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");

        // Create the package URI
        jstring PackageURIString = Env->NewStringUTF(TCHAR_TO_UTF8(*FString::Printf(TEXT("package:%s"), *PackageName)));
        jclass UriClass = Env->FindClass("android/net/Uri");
        jmethodID ParseMethod = Env->GetStaticMethodID(UriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
        jobject PackageURI = Env->CallStaticObjectMethod(UriClass, ParseMethod, PackageURIString);

        // Create the uninstall intent
        jstring ActionUninstall = Env->NewStringUTF("android.intent.action.UNINSTALL_PACKAGE");
        jobject UninstallIntent = Env->NewObject(IntentClass, IntentConstructor, ActionUninstall, PackageURI);

        // Add FLAG_ACTIVITY_NEW_TASK
        jmethodID AddFlagsMethod = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
        Env->CallObjectMethod(UninstallIntent, AddFlagsMethod, 0x10000000); // FLAG_ACTIVITY_NEW_TASK

        // Start the activity
        jmethodID StartActivity = Env->GetMethodID(ActivityClass, "startActivity", "(Landroid/content/Intent;)V");
        Env->CallVoidMethod(Activity, StartActivity, UninstallIntent);

        // Clean up local references
        Env->DeleteLocalRef(ActionUninstall);
        Env->DeleteLocalRef(PackageURIString);
        Env->DeleteLocalRef(PackageURI);
        Env->DeleteLocalRef(UninstallIntent);
        Env->DeleteLocalRef(UriClass);
        Env->DeleteLocalRef(IntentClass);
        Env->DeleteLocalRef(ActivityClass);
    }
    #endif
}

int32 UAPKInstallerLibrary::GetPackageVersionCode(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);
        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);
        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);
        jmethodID GetPackageInfo = Env->GetMethodID(PackageManagerClass, "getPackageInfo",
                                                    "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
        jstring JPackageName = Env->NewStringUTF(TCHAR_TO_UTF8(*PackageName));
        int32 VersionCode = -1;

        jobject PackageInfo = Env->CallObjectMethod(PackageManager, GetPackageInfo, JPackageName, 0);

        if (Env->ExceptionCheck())
        {
            Env->ExceptionClear(); // Package not found
        }
        else if (PackageInfo)
        {
            jclass PackageInfoClass = Env->GetObjectClass(PackageInfo);
            jfieldID VersionCodeField = Env->GetFieldID(PackageInfoClass, "versionCode", "I");
            VersionCode = Env->GetIntField(PackageInfo, VersionCodeField);
            Env->DeleteLocalRef(PackageInfoClass);
        }

        Env->DeleteLocalRef(JPackageName);
        if (PackageInfo) Env->DeleteLocalRef(PackageInfo);
        Env->DeleteLocalRef(PackageManagerClass);
        Env->DeleteLocalRef(PackageManager);
        Env->DeleteLocalRef(ActivityClass);

        return VersionCode;
    }
    #endif
    return -1;
}

FString UAPKInstallerLibrary::GetPackageVersionName(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);
        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);
        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);
        jmethodID GetPackageInfo = Env->GetMethodID(PackageManagerClass, "getPackageInfo",
                                                    "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
        jstring JPackageName = Env->NewStringUTF(TCHAR_TO_UTF8(*PackageName));
        FString VersionName = TEXT("");

        jobject PackageInfo = Env->CallObjectMethod(PackageManager, GetPackageInfo, JPackageName, 0);

        if (Env->ExceptionCheck())
        {
            Env->ExceptionClear(); // Package not found
        }
        else if (PackageInfo)
        {
            jclass PackageInfoClass = Env->GetObjectClass(PackageInfo);
            jfieldID VersionNameField = Env->GetFieldID(PackageInfoClass, "versionName", "Ljava/lang/String;");
            jstring JVersionName = (jstring)Env->GetObjectField(PackageInfo, VersionNameField);

            if (JVersionName)
            {
                const char* VersionNameChars = Env->GetStringUTFChars(JVersionName, nullptr);
                VersionName = FString(UTF8_TO_TCHAR(VersionNameChars));
                Env->ReleaseStringUTFChars(JVersionName, VersionNameChars);
                Env->DeleteLocalRef(JVersionName);
            }

            Env->DeleteLocalRef(PackageInfoClass);
        }

        Env->DeleteLocalRef(JPackageName);
        if (PackageInfo) Env->DeleteLocalRef(PackageInfo);
        Env->DeleteLocalRef(PackageManagerClass);
        Env->DeleteLocalRef(PackageManager);
        Env->DeleteLocalRef(ActivityClass);

        return VersionName;
    }
    #endif
    return TEXT("");
}
