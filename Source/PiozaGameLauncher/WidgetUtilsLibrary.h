#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "Components/Border.h"
#include "Components/ContentWidget.h"
#include "WidgetUtilsLibrary.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UWidgetUtilsLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Recursively get all children of a widget, including panels, borders, content widgets, etc. */
    UFUNCTION(BlueprintCallable, Category="UI|Utilities")
    static void GetAllChildrenRecursive(UWidget* ParentWidget, TArray<UWidget*>& OutWidgets);

    /** Recursively get all children of a widget filtered by a specific class */
    UFUNCTION(BlueprintCallable, Category="UI|Utilities")
    static void GetAllChildrenOfClassRecursive(UWidget* ParentWidget, TSubclassOf<UWidget> WidgetClass, TArray<UWidget*>& OutWidgets);
};
