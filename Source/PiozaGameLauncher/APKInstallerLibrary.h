// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "APKInstallerLibrary.generated.h"

UENUM(BlueprintType)
enum class EAPKInstallError : uint8
{
    Success = 0,
    NotAndroidPlatform = 1,
    NoJNIEnvironment = 2,
    NoInstallPermission = 3,
    FileNotFound = 4,
    FileProviderClassNotFound = 5,
    FileProviderMethodNotFound = 6,
    FileProviderURICreationFailed = 7,
    IntentCreationFailed = 8,
    ActivityStartFailed = 9
};

UCLASS()
class PIOZAGAMELAUNCHER_API UAPKInstallerLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Installs an APK file on Android device
     * @param APKFilePath - Full path to the APK file (e.g. /storage/emulated/0/Download/app.apk)
     * @return true if installation was launched successfully, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "APK Installer")
    static EAPKInstallError InstallAPK(const FString& APKFilePath, FString& OutErrorMessage);

    /**
     * Checks if the application has permission to install from unknown sources
     * @return true if the application has permission, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Android|APK Installer")
    static bool CanInstallFromUnknownSources();

    /**
     * Opens system settings to grant permission for installing from unknown sources
     */
    UFUNCTION(BlueprintCallable, Category = "Android|APK Installer")
    static void OpenInstallPermissionSettings();

    /**
     * Returns the Android application package name of the currently running app.
     *
     * Example output: "com.dsh.piozalauncher"
     *
     * @return The package name of the current Android application. Returns an empty string on non-Android platforms.
     */
    UFUNCTION(BlueprintPure, Category="APK Installer")
    static FString GetAppPackageName();

    /**
     * Checks if an Android app with the given package name is installed on the device.
     *
     * @param PackageName  The package name to check (e.g. "com.dsh.piozalauncher").
     * @return true if the app is installed, false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Android|APK Installer")
    static bool IsPackageInstalled(const FString& PackageName);

    /**
     * Launches an Android app with the specified package name.
     *
     * @param PackageName  The package name of the app to launch (e.g. "com.dsh.piozalauncher").
     * @return true if the launch intent was successfully sent, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category="Android|APK Installer")
    static bool LaunchApp(const FString& PackageName);

    /**
     * Opens system dialog to uninstall an application by package name
     * @param PackageName - The package name to uninstall
     */
    UFUNCTION(BlueprintCallable, Category = "Android|APK Installer")
    static void UninstallPackage(const FString& PackageName);

    UFUNCTION(BlueprintCallable, Category = "APK Installer")
    static int32 GetPackageVersionCode(const FString& PackageName);

    UFUNCTION(BlueprintCallable, Category = "APK Installer")
    static FString GetPackageVersionName(const FString& PackageName);

};
