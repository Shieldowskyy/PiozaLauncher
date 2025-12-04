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

    /**
     * Creates a Vector2D with specified X value, keeping Y unchanged.
     * @param Original - Original Vector2D.
     * @param NewX - New X value.
     * @return Modified Vector2D.
     */
    UFUNCTION(BlueprintPure, Category = "Math|Vector2D")
    static FVector2D SetVector2DX(const FVector2D& Original, float NewX);

    /**
     * Creates a Vector2D with specified Y value, keeping X unchanged.
     * @param Original - Original Vector2D.
     * @param NewY - New Y value.
     * @return Modified Vector2D.
     */
    UFUNCTION(BlueprintPure, Category = "Math|Vector2D")
    static FVector2D SetVector2DY(const FVector2D& Original, float NewY);

    /**
     * Interpolates only the Y component of Vector2D, keeping X unchanged.
     * @param Current - Current Vector2D.
     * @param Target - Target Vector2D.
     * @param DeltaTime - Time step.
     * @param InterpSpeed - Interpolation speed.
     * @return Interpolated Vector2D with X from Current.
     */
    UFUNCTION(BlueprintPure, Category = "Math|Vector2D")
    static FVector2D Vector2DInterpToY(
        const FVector2D& Current,
        const FVector2D& Target,
        float DeltaTime,
        float InterpSpeed);

    /**
     * Interpolates only the X component of Vector2D, keeping Y unchanged.
     * @param Current - Current Vector2D.
     * @param Target - Target Vector2D.
     * @param DeltaTime - Time step.
     * @param InterpSpeed - Interpolation speed.
     * @return Interpolated Vector2D with Y from Current.
     */
    UFUNCTION(BlueprintPure, Category = "Math|Vector2D")
    static FVector2D Vector2DInterpToX(
        const FVector2D& Current,
        const FVector2D& Target,
        float DeltaTime,
        float InterpSpeed);

    /**
     * Sets the Image Size for all button states.
     * @param ButtonStyle - The button style to modify.
     * @param ImageSize - The image size to apply to all states.
     * @return The modified button style.
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    static FButtonStyle SetButtonImageSize(
        const FButtonStyle& ButtonStyle,
        const FVector2D& ImageSize);

    /**
     * Sets the Image Size for all button states with individual values per state.
     * @param ButtonStyle - The button style to modify.
     * @param NormalSize - Image size for normal state.
     * @param HoveredSize - Image size for hovered state.
     * @param PressedSize - Image size for pressed state.
     * @param DisabledSize - Image size for disabled state.
     * @return The modified button style.
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    static FButtonStyle SetButtonImageSizePerState(
        const FButtonStyle& ButtonStyle,
        const FVector2D& NormalSize,
        const FVector2D& HoveredSize,
        const FVector2D& PressedSize,
        const FVector2D& DisabledSize);
};