// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

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
