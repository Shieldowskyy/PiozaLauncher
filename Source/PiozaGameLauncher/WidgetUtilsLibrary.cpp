// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "WidgetUtilsLibrary.h"

void UWidgetUtilsLibrary::GetAllChildrenRecursive(UWidget* ParentWidget, TArray<UWidget*>& OutWidgets)
{
    GetAllChildrenOfClassRecursive(ParentWidget, UWidget::StaticClass(), OutWidgets);
}

void UWidgetUtilsLibrary::GetAllChildrenOfClassRecursive(UWidget* ParentWidget, TSubclassOf<UWidget> WidgetClass, TArray<UWidget*>& OutWidgets)
{
    if (!ParentWidget) return;

    // Check if the current widget matches the filter
    // If WidgetClass is null, we assume we want everything.
    if (!WidgetClass || ParentWidget->IsA(WidgetClass))
    {
        OutWidgets.Add(ParentWidget);
    }

    // Recursion
    // UContentWidget inherits from UPanelWidget, so this cast handles both cases.
    if (UPanelWidget* Panel = Cast<UPanelWidget>(ParentWidget))
    {
        const int32 Count = Panel->GetChildrenCount();
        for (int32 i = 0; i < Count; i++)
        {
            if (UWidget* Child = Panel->GetChildAt(i))
            {
                GetAllChildrenOfClassRecursive(Child, WidgetClass, OutWidgets);
            }
        }
    }
}