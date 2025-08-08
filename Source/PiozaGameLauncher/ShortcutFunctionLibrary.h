#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShortcutFunctionLibrary.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UShortcutFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Creates a desktop shortcut.
     * @param ProgramPath Full path to the executable or target program.
     * @param ShortcutName Name of the shortcut file to create.
     * @param LaunchArgs Optional command line arguments to pass when launching the program.
     * @param IconPath Optional path to an icon file to be used for the shortcut.
     * @return true if the shortcut was created successfully, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "Shortcut")
    static bool CreateDesktopShortcut(const FString& ProgramPath, const FString& ShortcutName, const FString& LaunchArgs = TEXT(""), const FString& IconPath = TEXT(""));

    /**
     * Removes a desktop shortcut.
     * @param ShortcutName Name of the shortcut file to remove.
     * @return true if the shortcut was removed successfully, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "Shortcut")
    static bool RemoveDesktopShortcut(const FString& ShortcutName);

    /**
     * Creates a start menu shortcut.
     * @param ProgramPath Full path to the executable or target program.
     * @param ShortcutName Name of the shortcut file to create.
     * @param LaunchArgs Optional command line arguments to pass when launching the program.
     * @param IconPath Optional path to an icon file to be used for the shortcut.
     * @return true if the shortcut was created successfully, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "Shortcut")
    static bool CreateStartMenuShortcut(const FString& ProgramPath, const FString& ShortcutName, const FString& LaunchArgs = TEXT(""), const FString& IconPath = TEXT(""));

    /**
     * Removes a start menu shortcut.
     * @param ShortcutName Name of the shortcut file to remove.
     * @return true if the shortcut was removed successfully, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "Shortcut")
    static bool RemoveStartMenuShortcut(const FString& ShortcutName);
};
