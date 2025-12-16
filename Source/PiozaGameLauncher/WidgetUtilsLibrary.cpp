// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

#include "WidgetUtilsLibrary.h"

void UWidgetUtilsLibrary::GetAllChildrenRecursive(UWidget* ParentWidget, TArray<UWidget*>& OutWidgets)
{
    if (!ParentWidget) return;

    // Add the current widget to the output
    OutWidgets.Add(ParentWidget);

    // If the widget is a PanelWidget (e.g., VerticalBox, Canvas, Grid)
    if (UPanelWidget* Panel = Cast<UPanelWidget>(ParentWidget))
    {
        const int32 Count = Panel->GetChildrenCount();
        for (int32 i = 0; i < Count; i++)
        {
            UWidget* Child = Panel->GetChildAt(i);
            // Recursive call for each child
            GetAllChildrenRecursive(Child, OutWidgets);
        }
    }
    // If the widget is a ContentWidget (e.g., Border, Button content)
    else if (UContentWidget* Content = Cast<UContentWidget>(ParentWidget))
    {
        if (UWidget* Child = Content->GetContent())
        {
            // Recursive call for the single child
            GetAllChildrenRecursive(Child, OutWidgets);
        }
    }
}

void UWidgetUtilsLibrary::GetAllChildrenOfClassRecursive(UWidget* ParentWidget, TSubclassOf<UWidget> WidgetClass, TArray<UWidget*>& OutWidgets)
{
    if (!ParentWidget) return;

    // Add the current widget if it matches the class (or if no class filter is provided)
    if (!WidgetClass || ParentWidget->IsA(WidgetClass))
    {
        OutWidgets.Add(ParentWidget);
    }

    // If the widget is a PanelWidget
    if (UPanelWidget* Panel = Cast<UPanelWidget>(ParentWidget))
    {
        const int32 Count = Panel->GetChildrenCount();
        for (int32 i = 0; i < Count; i++)
        {
            UWidget* Child = Panel->GetChildAt(i);
            // Recursive call for each child
            GetAllChildrenOfClassRecursive(Child, WidgetClass, OutWidgets);
        }
    }
    // If the widget is a ContentWidget
    else if (UContentWidget* Content = Cast<UContentWidget>(ParentWidget))
    {
        if (UWidget* Child = Content->GetContent())
        {
            // Recursive call for the single child
            GetAllChildrenOfClassRecursive(Child, WidgetClass, OutWidgets);
        }
    }
}
