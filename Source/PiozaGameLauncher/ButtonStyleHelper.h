#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Styling/SlateTypes.h"
#include "ButtonStyleHelper.generated.h"

/**
 * Klasa pomocnicza do modyfikacji stylów przycisków.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UButtonStyleHelper : public UObject
{
	GENERATED_BODY()

public:

	/** 
	 * Ustawia ten sam kolor tła dla wszystkich stanów przycisku.
	 * @param ButtonStyle - Styl, który ma być zmodyfikowany.
	 * @param BackgroundColor - Kolor tła do ustawienia.
	 * @return Zmodyfikowany styl przycisku.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	static FButtonStyle SetButtonBackgroundColorWithAlpha(
		const FButtonStyle& ButtonStyle,
		const FLinearColor& BackgroundColor,
		float NormalAlpha = 1.0f,
		float HoveredAlpha = 0.8f,
		float PressedAlpha = 0.6f,
		float DisabledAlpha = 0.4f);
};
