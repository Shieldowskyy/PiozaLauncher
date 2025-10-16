#include "APKInstallerLibrary.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#endif

bool UAPKInstallerLibrary::InstallAPK(const FString& APKFilePath)
{
    #if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        // Get the activity
        jobject Activity = FAndroidApplication::GetGameActivityThis();
        jclass ActivityClass = Env->GetObjectClass(Activity);

        // Check permissions first
        jmethodID GetPackageManager = Env->GetMethodID(ActivityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
        jobject PackageManager = Env->CallObjectMethod(Activity, GetPackageManager);

        jclass PackageManagerClass = Env->GetObjectClass(PackageManager);
        jmethodID CanRequestPackageInstalls = Env->GetMethodID(PackageManagerClass, "canRequestPackageInstalls", "()Z");

        if (CanRequestPackageInstalls != nullptr)
        {
            jboolean CanInstall = Env->CallBooleanMethod(PackageManager, CanRequestPackageInstalls);

            if (!CanInstall)
            {
                UE_LOG(LogTemp, Warning, TEXT("No permission to install APK. Use OpenInstallPermissionSettings()."));
                Env->DeleteLocalRef(ActivityClass);
                Env->DeleteLocalRef(PackageManager);
                Env->DeleteLocalRef(PackageManagerClass);
                return false;
            }
        }

        Env->DeleteLocalRef(PackageManager);
        Env->DeleteLocalRef(PackageManagerClass);

        // Get Android SDK version
        jclass VersionClass = Env->FindClass("android/os/Build$VERSION");
        jfieldID SdkIntField = Env->GetStaticFieldID(VersionClass, "SDK_INT", "I");
        jint SdkInt = Env->GetStaticIntField(VersionClass, SdkIntField);
        Env->DeleteLocalRef(VersionClass);

        UE_LOG(LogTemp, Log, TEXT("Android SDK version: %d"), SdkInt);

        // Prepare file path
        jstring JFilePath = Env->NewStringUTF(TCHAR_TO_UTF8(*APKFilePath));

        // Create File object
        jclass FileClass = Env->FindClass("java/io/File");
        jmethodID FileConstructor = Env->GetMethodID(FileClass, "<init>", "(Ljava/lang/String;)V");
        jobject FileObject = Env->NewObject(FileClass, FileConstructor, JFilePath);

        // Check if file exists
        jmethodID ExistsMethod = Env->GetMethodID(FileClass, "exists", "()Z");
        jboolean FileExists = Env->CallBooleanMethod(FileObject, ExistsMethod);

        if (!FileExists)
        {
            UE_LOG(LogTemp, Error, TEXT("APK file does not exist: %s"), *APKFilePath);
            Env->DeleteLocalRef(JFilePath);
            Env->DeleteLocalRef(FileClass);
            Env->DeleteLocalRef(FileObject);
            Env->DeleteLocalRef(ActivityClass);
            return false;
        }

        // Create Intent
        jclass IntentClass = Env->FindClass("android/content/Intent");
        jmethodID IntentConstructor = Env->GetMethodID(IntentClass, "<init>", "(Ljava/lang/String;)V");
        jstring ActionInstall = Env->NewStringUTF("android.intent.action.INSTALL_PACKAGE");
        jobject Intent = Env->NewObject(IntentClass, IntentConstructor, ActionInstall);

        jobject Uri = nullptr;

        // For Android 7.0+ (API 24+), try to use FileProvider
        if (SdkInt >= 24)
        {
            // Get application context
            jmethodID GetApplicationContext = Env->GetMethodID(ActivityClass, "getApplicationContext", "()Landroid/content/Context;");
            jobject Context = Env->CallObjectMethod(Activity, GetApplicationContext);

            // Get package name
            jmethodID GetPackageName = Env->GetMethodID(ActivityClass, "getPackageName", "()Ljava/lang/String;");
            jstring PackageName = (jstring)Env->CallObjectMethod(Activity, GetPackageName);

            // Create authority string
            const char* PackageNameStr = Env->GetStringUTFChars(PackageName, nullptr);
            FString Authority = FString(PackageNameStr) + TEXT(".fileprovider");
            Env->ReleaseStringUTFChars(PackageName, PackageNameStr);

            jstring JAuthority = Env->NewStringUTF(TCHAR_TO_UTF8(*Authority));

            UE_LOG(LogTemp, Log, TEXT("Trying FileProvider with authority: %s"), *Authority);

            // Try to find FileProvider class using ClassLoader
            jclass ContextClass = Env->GetObjectClass(Context);
            jmethodID GetClassLoader = Env->GetMethodID(ContextClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
            jobject ClassLoader = Env->CallObjectMethod(Context, GetClassLoader);

            jclass ClassLoaderClass = Env->GetObjectClass(ClassLoader);
            jmethodID LoadClass = Env->GetMethodID(ClassLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

            jstring FileProviderClassName = Env->NewStringUTF("androidx.core.content.FileProvider");
            jclass FileProviderClass = (jclass)Env->CallObjectMethod(ClassLoader, LoadClass, FileProviderClassName);

            Env->DeleteLocalRef(ContextClass);
            Env->DeleteLocalRef(ClassLoader);
            Env->DeleteLocalRef(ClassLoaderClass);
            Env->DeleteLocalRef(FileProviderClassName);

            if (FileProviderClass != nullptr && !Env->ExceptionCheck())
            {
                // Get getUriForFile method
                jmethodID GetUriForFile = Env->GetStaticMethodID(FileProviderClass, "getUriForFile",
                                                                 "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;");

                if (GetUriForFile != nullptr && !Env->ExceptionCheck())
                {
                    Uri = Env->CallStaticObjectMethod(FileProviderClass, GetUriForFile, Context, JAuthority, FileObject);

                    if (Env->ExceptionCheck())
                    {
                        Env->ExceptionDescribe();
                        Env->ExceptionClear();
                        Uri = nullptr;
                        UE_LOG(LogTemp, Warning, TEXT("FileProvider.getUriForFile failed, falling back to file:// URI"));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Log, TEXT("FileProvider URI created successfully"));
                    }
                }
                else
                {
                    Env->ExceptionClear();
                    UE_LOG(LogTemp, Warning, TEXT("FileProvider.getUriForFile method not found"));
                }

                Env->DeleteLocalRef(FileProviderClass);
            }
            else
            {
                Env->ExceptionClear();
                UE_LOG(LogTemp, Warning, TEXT("FileProvider class not found, using fallback"));
            }

            Env->DeleteLocalRef(Context);
            Env->DeleteLocalRef(PackageName);
            Env->DeleteLocalRef(JAuthority);
        }

        // Fallback: Create file:// URI for older Android or if FileProvider failed
        if (Uri == nullptr)
        {
            UE_LOG(LogTemp, Log, TEXT("Using file:// URI (fallback)"));

            jclass UriClass = Env->FindClass("android/net/Uri");
            jmethodID FromFileMethod = Env->GetStaticMethodID(UriClass, "fromFile", "(Ljava/io/File;)Landroid/net/Uri;");
            Uri = Env->CallStaticObjectMethod(UriClass, FromFileMethod, FileObject);
            Env->DeleteLocalRef(UriClass);
        }

        if (Uri != nullptr)
        {
            // Set data and type
            jmethodID SetDataAndType = Env->GetMethodID(IntentClass, "setDataAndType",
                                                        "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;");
            jstring MimeType = Env->NewStringUTF("application/vnd.android.package-archive");
            Env->CallObjectMethod(Intent, SetDataAndType, Uri, MimeType);

            // Add flags
            jmethodID AddFlags = Env->GetMethodID(IntentClass, "addFlags", "(I)Landroid/content/Intent;");
            Env->CallObjectMethod(Intent, AddFlags, 0x00000001); // FLAG_GRANT_READ_URI_PERMISSION
            Env->CallObjectMethod(Intent, AddFlags, 0x10000000); // FLAG_ACTIVITY_NEW_TASK

            // Start activity
            jmethodID StartActivity = Env->GetMethodID(ActivityClass, "startActivity", "(Landroid/content/Intent;)V");
            Env->CallVoidMethod(Activity, StartActivity, Intent);

            if (Env->ExceptionCheck())
            {
                Env->ExceptionDescribe();
                Env->ExceptionClear();
                UE_LOG(LogTemp, Error, TEXT("Failed to start installation activity"));

                Env->DeleteLocalRef(MimeType);
                Env->DeleteLocalRef(Uri);
                Env->DeleteLocalRef(JFilePath);
                Env->DeleteLocalRef(FileClass);
                Env->DeleteLocalRef(FileObject);
                Env->DeleteLocalRef(ActivityClass);
                Env->DeleteLocalRef(IntentClass);
                Env->DeleteLocalRef(Intent);
                Env->DeleteLocalRef(ActionInstall);
                return false;
            }

            UE_LOG(LogTemp, Log, TEXT("APK installation started successfully"));

            // Cleanup
            Env->DeleteLocalRef(MimeType);
            Env->DeleteLocalRef(Uri);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create URI for APK file"));
            Env->DeleteLocalRef(JFilePath);
            Env->DeleteLocalRef(FileClass);
            Env->DeleteLocalRef(FileObject);
            Env->DeleteLocalRef(ActivityClass);
            Env->DeleteLocalRef(IntentClass);
            Env->DeleteLocalRef(Intent);
            Env->DeleteLocalRef(ActionInstall);
            return false;
        }

        // Cleanup
        Env->DeleteLocalRef(JFilePath);
        Env->DeleteLocalRef(FileClass);
        Env->DeleteLocalRef(FileObject);
        Env->DeleteLocalRef(ActivityClass);
        Env->DeleteLocalRef(IntentClass);
        Env->DeleteLocalRef(Intent);
        Env->DeleteLocalRef(ActionInstall);

        return true;
    }
    #endif

    UE_LOG(LogTemp, Warning, TEXT("InstallAPK only works on Android"));
    return false;
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

