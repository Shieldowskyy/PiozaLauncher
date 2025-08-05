#include "WindowUtils.h"
#include "Framework/Application/SlateApplication.h"

void UWindowUtils::MinimizeWindow()
{
    TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow();
    if (Window.IsValid())
    {
        Window->Minimize();
    }
}

void UWindowUtils::RestoreWindow()
{
    TSharedPtr<SWindow> Window = FSlateApplication::Get().GetActiveTopLevelWindow();
    if (Window.IsValid())
    {
        Window->Restore();
    }
}
