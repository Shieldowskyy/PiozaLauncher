#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShortcutFunctionLibrary.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UShortcutFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Creates a desktop shortcut with optional launch arguments and icon path */
    UFUNCTION(BlueprintCallable, Category = "Shortcut")
    static bool CreateDesktopShortcut(FString ProgramPath, FString ShortcutName, FString LaunchArgs, FString IconPath = TEXT(""));

    /** Removes a desktop shortcut */
    UFUNCTION(BlueprintCallable, Category = "Shortcut")
    static bool RemoveDesktopShortcut(FString ShortcutName);

    /** Creates a start menu shortcut with optional launch arguments and icon path */
    UFUNCTION(BlueprintCallable, Category = "Shortcut")
    static bool CreateStartMenuShortcut(FString ProgramPath, FString ShortcutName, FString LaunchArgs, FString IconPath = TEXT(""));

    /** Removes a start menu shortcut */
    UFUNCTION(BlueprintCallable, Category = "Shortcut")
    static bool RemoveStartMenuShortcut(FString ShortcutName);
};
