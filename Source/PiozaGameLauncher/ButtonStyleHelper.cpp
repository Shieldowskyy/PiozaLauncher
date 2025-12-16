// Pioza Launcher
// Copyright (c) 2025 DashoGames
// Licensed under the MIT License - see LICENSE file for details

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

FButtonStyle UButtonStyleHelper::SetButtonOutlineSettings(
	const FButtonStyle& ButtonStyle,
	const FSlateBrushOutlineSettings& OutlineSettings)
{
	FButtonStyle NewStyle = ButtonStyle;

	NewStyle.Normal.OutlineSettings = OutlineSettings;
	NewStyle.Hovered.OutlineSettings = OutlineSettings;
	NewStyle.Pressed.OutlineSettings = OutlineSettings;
	NewStyle.Disabled.OutlineSettings = OutlineSettings;

	return NewStyle;
}

FVector2D UButtonStyleHelper::SetVector2DX(const FVector2D& Original, float NewX)
{
	return FVector2D(NewX, Original.Y);
}

FVector2D UButtonStyleHelper::SetVector2DY(const FVector2D& Original, float NewY)
{
	return FVector2D(Original.X, NewY);
}

FVector2D UButtonStyleHelper::Vector2DInterpToY(
	const FVector2D& Current,
	const FVector2D& Target,
	float DeltaTime,
	float InterpSpeed)
{
	float NewY = FMath::FInterpTo(Current.Y, Target.Y, DeltaTime, InterpSpeed);
	return FVector2D(Current.X, NewY);
}

FVector2D UButtonStyleHelper::Vector2DInterpToX(
	const FVector2D& Current,
	const FVector2D& Target,
	float DeltaTime,
	float InterpSpeed)
{
	float NewX = FMath::FInterpTo(Current.X, Target.X, DeltaTime, InterpSpeed);
	return FVector2D(NewX, Current.Y);
}

FButtonStyle UButtonStyleHelper::SetButtonImageSize(
	const FButtonStyle& ButtonStyle,
	const FVector2D& ImageSize)
{
	FButtonStyle NewStyle = ButtonStyle;

	NewStyle.Normal.ImageSize = ImageSize;
	NewStyle.Hovered.ImageSize = ImageSize;
	NewStyle.Pressed.ImageSize = ImageSize;
	NewStyle.Disabled.ImageSize = ImageSize;

	return NewStyle;
}

FButtonStyle UButtonStyleHelper::SetButtonImageSizePerState(
	const FButtonStyle& ButtonStyle,
	const FVector2D& NormalSize,
	const FVector2D& HoveredSize,
	const FVector2D& PressedSize,
	const FVector2D& DisabledSize)
{
	FButtonStyle NewStyle = ButtonStyle;

	NewStyle.Normal.ImageSize = NormalSize;
	NewStyle.Hovered.ImageSize = HoveredSize;
	NewStyle.Pressed.ImageSize = PressedSize;
	NewStyle.Disabled.ImageSize = DisabledSize;

	return NewStyle;
}