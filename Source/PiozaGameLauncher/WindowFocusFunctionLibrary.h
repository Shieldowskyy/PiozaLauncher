#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "WindowFocusFunctionLibrary.generated.h"

UCLASS()
class PIOZAGAMELAUNCHER_API UWindowFocusFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Checks if game window has focus. */
	UFUNCTION(BlueprintPure, Category = "Window")
	static bool IsGameWindowFocused();
};
