// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "APKUtilsLibrary.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#endif

FString UAPKUtilsLibrary::GetAppPackageName()
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        if (!Activity) return TEXT("");

        jclass ActivityClass = Env->GetObjectClass(Activity);
        jmethodID GetPackageName = Env->GetMethodID(ActivityClass, "getPackageName", "()Ljava/lang/String;");

        jstring PackageName = (jstring)Env->CallObjectMethod(Activity, GetPackageName);
        FString Result;

        if (PackageName)
        {
            const char* PackageNameChars = Env->GetStringUTFChars(PackageName, nullptr);
            Result = UTF8_TO_TCHAR(PackageNameChars);
            Env->ReleaseStringUTFChars(PackageName, PackageNameChars);
            Env->DeleteLocalRef(PackageName);
        }
        Env->DeleteLocalRef(ActivityClass);
        return Result;
    }
    #endif
    return TEXT("");
}

bool UAPKUtilsLibrary::IsPackageInstalled(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);

        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);
        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);

        jmethodID GetPackageInfo = Env->GetMethodID(PackageManagerClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
        jstring JPackageName = Env->NewStringUTF(TCHAR_TO_UTF8(*PackageName));

        bool bInstalled = true;
        jobject PackageInfo = Env->CallObjectMethod(PackageManager, GetPackageInfo, JPackageName, 0);

        if (Env->ExceptionCheck())
        {
            Env->ExceptionClear();
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

bool UAPKUtilsLibrary::LaunchApp(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);

        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);
        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);

        jmethodID GetLaunchIntent = Env->GetMethodID(PackageManagerClass, "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;");
        jstring JPackageName = Env->NewStringUTF(TCHAR_TO_UTF8(*PackageName));

        jobject LaunchIntent = Env->CallObjectMethod(PackageManager, GetLaunchIntent, JPackageName);

        bool bLaunched = false;
        if (LaunchIntent != nullptr)
        {
            jclass IntentClass = Env->GetObjectClass(LaunchIntent);
            jmethodID AddFlags = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
            Env->CallObjectMethod(LaunchIntent, AddFlags, 0x10000000);

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

int32 UAPKUtilsLibrary::GetPackageVersionCode(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);
        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);
        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);
        jmethodID GetPackageInfo = Env->GetMethodID(PackageManagerClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
        jstring JPackageName = Env->NewStringUTF(TCHAR_TO_UTF8(*PackageName));
        int32 VersionCode = -1;

        jobject PackageInfo = Env->CallObjectMethod(PackageManager, GetPackageInfo, JPackageName, 0);

        if (Env->ExceptionCheck())
        {
            Env->ExceptionClear();
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

FString UAPKUtilsLibrary::GetPackageVersionName(const FString& PackageName)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);
        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);
        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);
        jmethodID GetPackageInfo = Env->GetMethodID(PackageManagerClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
        jstring JPackageName = Env->NewStringUTF(TCHAR_TO_UTF8(*PackageName));
        FString VersionName = TEXT("");

        jobject PackageInfo = Env->CallObjectMethod(PackageManager, GetPackageInfo, JPackageName, 0);

        if (Env->ExceptionCheck())
        {
            Env->ExceptionClear();
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
