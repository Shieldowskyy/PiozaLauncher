#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

#include "ShortcutFunctionLibrary.generated.h"

/**
 * UShortcutFunctionLibrary
 * A function library for creating shortcuts on Windows and Linux systems
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UShortcutFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Function to create a shortcut on the desktop
    UFUNCTION(BlueprintCallable, Category = "Utilities")
    static bool CreateDesktopShortcut(FString ProgramPath, FString ShortcutName);

    // Function to create a shortcut in the Start Menu (Windows) or in applications (Linux)
    UFUNCTION(BlueprintCallable, Category = "Utilities")
    static bool CreateStartMenuShortcut(FString ProgramPath, FString ShortcutName);

    // Function to remove a shortcut from the desktop
    UFUNCTION(BlueprintCallable, Category = "Utilities")
    static bool RemoveDesktopShortcut(FString ShortcutName);

    // Function to remove a shortcut from the Start Menu (Windows) or applications (Linux)
    UFUNCTION(BlueprintCallable, Category = "Utilities")
    static bool RemoveStartMenuShortcut(FString ShortcutName);
};
