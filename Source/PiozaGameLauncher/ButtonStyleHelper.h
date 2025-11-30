#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "ButtonStyleHelper.generated.h"

/**
 * Helper class for modifying button styles.
 */
UCLASS()
class PIOZAGAMELAUNCHER_API UButtonStyleHelper : public UObject
{
    GENERATED_BODY()

public:

    /**
     * Sets the same background color for all button states.
     * @param ButtonStyle - The button style to modify.
     * @param BackgroundColor - The background color to apply.
     * @param NormalAlpha - Alpha for the normal state (default 1.0).
     * @param HoveredAlpha - Alpha for the hovered state (default 0.8).
     * @param PressedAlpha - Alpha for the pressed state (default 0.6).
     * @param DisabledAlpha - Alpha for the disabled state (default 0.4).
     * @return The modified button style.
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    static FButtonStyle SetButtonBackgroundColorWithAlpha(
        const FButtonStyle& ButtonStyle,
        const FLinearColor& BackgroundColor,
        float NormalAlpha = 1.0f,
        float HoveredAlpha = 0.8f,
        float PressedAlpha = 0.6f,
        float DisabledAlpha = 0.4f);

    /**
     * Sets outline settings for all button states using FSlateBrushOutlineSettings.
     * @param ButtonStyle - The button style to modify.
     * @param OutlineSettings - Outline settings to apply to all states.
     * @return The modified button style.
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    static FButtonStyle SetButtonOutlineSettings(
        const FButtonStyle& ButtonStyle,
        const FSlateBrushOutlineSettings& OutlineSettings);
};