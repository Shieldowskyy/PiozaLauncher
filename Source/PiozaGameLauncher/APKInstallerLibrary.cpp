// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "APKInstallerLibrary.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"

static FString GetFileProviderAuthority(JNIEnv* Env, jobject Activity)
{
    // Get Package Name
    jclass ActivityClass = Env->GetObjectClass(Activity);
    jmethodID GetPackageName = Env->GetMethodID(ActivityClass, "getPackageName", "()Ljava/lang/String;");
    jstring PackageName = (jstring)Env->CallObjectMethod(Activity, GetPackageName);

    const char* PackageNameChars = Env->GetStringUTFChars(PackageName, nullptr);
    FString Auth = FString(UTF8_TO_TCHAR(PackageNameChars)) + TEXT(".fileprovider");

    // Cleanup
    Env->ReleaseStringUTFChars(PackageName, PackageNameChars);
    Env->DeleteLocalRef(PackageName);
    Env->DeleteLocalRef(ActivityClass);

    return Auth;
}
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

    // --- SCOPE MANAGEMENT FOR LOCAL REFS ---
    if (Env->PushLocalFrame(128) < 0)
    {
        OutErrorMessage = TEXT("Failed to push JNI local frame");
        return EAPKInstallError::UnknownError; // Teraz to zadziała, bo dodaliśmy wartość do Enuma
    }

    // 1. Get ClassLoader & FileProvider Class
    jclass ActivityClass = Env->GetObjectClass(Activity);
    jmethodID GetClassLoader = Env->GetMethodID(ActivityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject Loader = Env->CallObjectMethod(Activity, GetClassLoader);

    jclass ClassClass = Env->FindClass("java/lang/Class");
    jmethodID ForNameMethod = Env->GetStaticMethodID(ClassClass, "forName", "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");

    jstring FileProviderName = Env->NewStringUTF("androidx.core.content.FileProvider");
    jclass FileProviderClass = (jclass)Env->CallStaticObjectMethod(ClassClass, ForNameMethod, FileProviderName, JNI_TRUE, Loader);

    if (!FileProviderClass)
    {
        Env->PopLocalFrame(nullptr);
        OutErrorMessage = TEXT("Failed to find androidx.core.content.FileProvider. Ensure AndroidX is enabled.");
        return EAPKInstallError::FileProviderClassNotFound;
    }

    // 2. Prepare File Object
    jstring JFilePath = Env->NewStringUTF(TCHAR_TO_UTF8(*APKFilePath));
    jclass FileClass = Env->FindClass("java/io/File");
    jmethodID FileConstructor = Env->GetMethodID(FileClass, "<init>", "(Ljava/lang/String;)V");
    jobject FileObject = Env->NewObject(FileClass, FileConstructor, JFilePath);

    jmethodID ExistsMethod = Env->GetMethodID(FileClass, "exists", "()Z");
    if (!Env->CallBooleanMethod(FileObject, ExistsMethod))
    {
        Env->PopLocalFrame(nullptr);
        OutErrorMessage = FString::Printf(TEXT("APK file does not exist: %s"), *APKFilePath);
        return EAPKInstallError::FileNotFound;
    }

    // 3. Get Context
    jmethodID GetApplicationContext = Env->GetMethodID(ActivityClass, "getApplicationContext", "()Landroid/content/Context;");
    jobject Context = Env->CallObjectMethod(Activity, GetApplicationContext);

    // 4. Get Dynamic Authority
    FString AuthorityString = GetFileProviderAuthority(Env, Activity);
    jstring JAuthority = Env->NewStringUTF(TCHAR_TO_UTF8(*AuthorityString));

    // 5. Get URI using FileProvider
    jmethodID GetUriForFile = Env->GetStaticMethodID(FileProviderClass, "getUriForFile", "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;");
    jobject Uri = Env->CallStaticObjectMethod(FileProviderClass, GetUriForFile, Context, JAuthority, FileObject);

    if (Env->ExceptionCheck())
    {
        Env->ExceptionDescribe();
        Env->ExceptionClear();
        Env->PopLocalFrame(nullptr);
        OutErrorMessage = TEXT("FileProvider exception. Check res/xml/filepaths.xml configuration.");
        return EAPKInstallError::FileProviderURICreationFailed;
    }

    if (!Uri)
    {
        Env->PopLocalFrame(nullptr);
        OutErrorMessage = TEXT("Failed to get URI (null).");
        return EAPKInstallError::FileProviderURICreationFailed;
    }

    // 6. Create Intent
    jclass IntentClass = Env->FindClass("android/content/Intent");
    jmethodID IntentConstructor = Env->GetMethodID(IntentClass, "<init>", "(Ljava/lang/String;)V");
    jstring ActionInstall = Env->NewStringUTF("android.intent.action.INSTALL_PACKAGE");
    jobject Intent = Env->NewObject(IntentClass, IntentConstructor, ActionInstall);

    jmethodID SetDataAndType = Env->GetMethodID(IntentClass, "setDataAndType", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;");
    jstring MimeType = Env->NewStringUTF("application/vnd.android.package-archive");
    Env->CallObjectMethod(Intent, SetDataAndType, Uri, MimeType);

    // 7. Add Flags
    jmethodID AddFlags = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
    Env->CallObjectMethod(Intent, AddFlags, 0x00000001); // FLAG_GRANT_READ_URI_PERMISSION
    Env->CallObjectMethod(Intent, AddFlags, 0x10000000); // FLAG_ACTIVITY_NEW_TASK

    // 8. Start Activity
    jmethodID StartActivity = Env->GetMethodID(ActivityClass, "startActivity", "(Landroid/content/Intent;)V");
    Env->CallVoidMethod(Activity, StartActivity, Intent);

    // 9. Cleanup
    Env->PopLocalFrame(nullptr);

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

        jclass IntentClass = Env->FindClass("android/content/Intent");
        jmethodID IntentConstructor = Env->GetMethodID(IntentClass, "<init>", "(Ljava/lang/String;)V");

        jstring ActionManageUnknownAppSources = Env->NewStringUTF("android.settings.MANAGE_UNKNOWN_APP_SOURCES");
        jobject Intent = Env->NewObject(IntentClass, IntentConstructor, ActionManageUnknownAppSources);

        jmethodID AddFlags = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
        Env->CallObjectMethod(Intent, AddFlags, 0x10000000);

        jmethodID StartActivity = Env->GetMethodID(ActivityClass, "startActivity", "(Landroid/content/Intent;)V");
        Env->CallVoidMethod(Activity, StartActivity, Intent);

        Env->DeleteLocalRef(ActivityClass);
        Env->DeleteLocalRef(IntentClass);
        Env->DeleteLocalRef(Intent);
        Env->DeleteLocalRef(ActionManageUnknownAppSources);
    }
    #endif
}

void UAPKInstallerLibrary::UninstallPackage(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);

        jclass IntentClass = Env->FindClass("android/content/Intent");
        jmethodID IntentConstructor = Env->GetMethodID(IntentClass, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V");

        jstring PackageURIString = Env->NewStringUTF(TCHAR_TO_UTF8(*FString::Printf(TEXT("package:%s"), *PackageName)));
        jclass UriClass = Env->FindClass("android/net/Uri");
        jmethodID ParseMethod = Env->GetStaticMethodID(UriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
        jobject PackageURI = Env->CallStaticObjectMethod(UriClass, ParseMethod, PackageURIString);

        jstring ActionUninstall = Env->NewStringUTF("android.intent.action.UNINSTALL_PACKAGE");
        jobject UninstallIntent = Env->NewObject(IntentClass, IntentConstructor, ActionUninstall, PackageURI);

        jmethodID AddFlagsMethod = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
        Env->CallObjectMethod(UninstallIntent, AddFlagsMethod, 0x10000000);

        jmethodID StartActivity = Env->GetMethodID(ActivityClass, "startActivity", "(Landroid/content/Intent;)V");
        Env->CallVoidMethod(Activity, StartActivity, UninstallIntent);

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

