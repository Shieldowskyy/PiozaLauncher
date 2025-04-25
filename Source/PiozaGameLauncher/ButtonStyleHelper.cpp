#include "ButtonStyleHelper.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"

FButtonStyle UButtonStyleHelper::SetButtonBackgroundColorWithAlpha(
	const FButtonStyle& ButtonStyle,
	const FLinearColor& BackgroundColor,
	float NormalAlpha,
	float HoveredAlpha,
	float PressedAlpha,
	float DisabledAlpha)
{
	FButtonStyle NewStyle = ButtonStyle;

	auto WithAlpha = [](const FLinearColor& Color, float Alpha) -> FSlateColor
	{
		return FSlateColor(FLinearColor(Color.R, Color.G, Color.B, Alpha));
	};

	NewStyle.Normal.TintColor = WithAlpha(BackgroundColor, NormalAlpha);
	NewStyle.Hovered.TintColor = WithAlpha(BackgroundColor, HoveredAlpha);
	NewStyle.Pressed.TintColor = WithAlpha(BackgroundColor, PressedAlpha);
	NewStyle.Disabled.TintColor = WithAlpha(BackgroundColor, DisabledAlpha);

	return NewStyle;
}

