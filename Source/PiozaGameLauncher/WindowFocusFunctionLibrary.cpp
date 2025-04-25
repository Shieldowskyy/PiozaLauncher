#include "WindowFocusFunctionLibrary.h"
#include "Framework/Application/SlateApplication.h"

bool UWindowFocusFunctionLibrary::IsGameWindowFocused()
{
	if (FSlateApplication::IsInitialized())
	{
		return FSlateApplication::Get().IsActive();
	}

	return false;
}
