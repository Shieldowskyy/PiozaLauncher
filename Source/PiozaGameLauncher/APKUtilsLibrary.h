// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "APKUtilsLibrary.generated.h"

/**
 * General Android package utilities for Pioza Launcher
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UAPKUtilsLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Returns the Android application package name of the currently running app.
     *
     * Example output: "com.dsh.piozalauncher"
     *
     * @return The package name of the current Android application. Returns an empty string on non-Android platforms.
     */
    UFUNCTION(BlueprintPure, Category="Android|APK Utils")
    static FString GetAppPackageName();

    /**
     * Checks if an Android app with the given package name is installed on the device.
     *
     * @param PackageName  The package name to check (e.g. "com.dsh.piozalauncher").
     * @return true if the app is installed, false otherwise.
     */
    UFUNCTION(BlueprintPure, Category="Android|APK Utils")
    static bool IsPackageInstalled(const FString& PackageName);

    /**
     * Launches an Android app with the specified package name.
     *
     * @param PackageName  The package name of the app to launch (e.g. "com.dsh.piozalauncher").
     * @return true if the launch intent was successfully sent, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category="Android|APK Utils")
    static bool LaunchApp(const FString& PackageName);

    /**
     * Returns the version code of the specified Android package.
     * @param PackageName - The package name to check
     * @return The version code, or -1 if the package is not found
     */
    UFUNCTION(BlueprintCallable, Category = "Android|APK Utils")
    static int32 GetPackageVersionCode(const FString& PackageName);

    /**
     * Returns the version name of the specified Android package.
     * @param PackageName - The package name to check
     * @return The version name, or empty string if the package is not found
     */
    UFUNCTION(BlueprintCallable, Category = "Android|APK Utils")
    static FString GetPackageVersionName(const FString& PackageName);
};
