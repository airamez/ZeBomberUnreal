// Copyright Epic Games, Inc. All Rights Reserved.

#include "FighterHUD.h"
#include "FighterPawn.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"

AFighterHUD::AFighterHUD()
{
	// Load the default Roboto font used by UE HUD
	static ConstructorHelpers::FObjectFinder<UFont> FontObj(TEXT("/Engine/EngineFonts/Roboto"));
	if (FontObj.Succeeded())
	{
		HUDFont = FontObj.Object;
	}
}

void AFighterHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	AFighterPawn* Fighter = Cast<AFighterPawn>(PC->GetPawn());
	if (!Fighter) return;

	// ==================== RED Bomb Impact Crosshair ====================
	{
		FVector BombImpact = Fighter->GetBombImpactPoint();

		// Project world point to screen
		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(BombImpact, ScreenPos, true))
		{
			// Draw circle crosshair
			DrawCircle(ScreenPos.X, ScreenPos.Y, BombCrosshairRadius, BombCircleSegments, BombCrosshairColor, BombCrosshairThickness);

			// Draw inner cross lines through the circle
			float HalfSize = BombCrosshairRadius * 0.6f;
			FLinearColor Col = BombCrosshairColor;
			float T = BombCrosshairThickness;

			// Horizontal line
			Canvas->K2_DrawLine(
				FVector2D(ScreenPos.X - HalfSize, ScreenPos.Y),
				FVector2D(ScreenPos.X + HalfSize, ScreenPos.Y),
				T, Col);

			// Vertical line
			Canvas->K2_DrawLine(
				FVector2D(ScreenPos.X, ScreenPos.Y - HalfSize),
				FVector2D(ScreenPos.X, ScreenPos.Y + HalfSize),
				T, Col);
		}
	}

	// ==================== WHITE Rocket Aim Crosshair (virtual cursor) ====================
	{
		FVector2D CursorPos = Fighter->GetCursorScreenPosition();

		DrawCrosshairPlus(CursorPos.X, CursorPos.Y, RocketCrosshairSize, RocketCrosshairGap, RocketCrosshairColor, RocketCrosshairThickness);

		// Center dot
		if (RocketCenterDotRadius > 0.0f)
		{
			DrawCircle(CursorPos.X, CursorPos.Y, RocketCenterDotRadius, 8, RocketCrosshairColor, RocketCrosshairThickness);
		}
	}

	// ==================== HUD Text ====================
	DrawSettingsInfo(Fighter);
	DrawScoreInfo(Fighter);
}

void AFighterHUD::DrawCircle(float CenterX, float CenterY, float Radius, int32 Segments, FLinearColor Color, float Thickness)
{
	if (Segments < 3) Segments = 3;

	float AngleStep = 2.0f * PI / static_cast<float>(Segments);

	for (int32 i = 0; i < Segments; ++i)
	{
		float Angle1 = AngleStep * i;
		float Angle2 = AngleStep * (i + 1);

		FVector2D P1(CenterX + Radius * FMath::Cos(Angle1), CenterY + Radius * FMath::Sin(Angle1));
		FVector2D P2(CenterX + Radius * FMath::Cos(Angle2), CenterY + Radius * FMath::Sin(Angle2));

		Canvas->K2_DrawLine(P1, P2, Thickness, Color);
	}
}

void AFighterHUD::DrawCrosshairPlus(float CenterX, float CenterY, float Size, float Gap, FLinearColor Color, float Thickness)
{
	// Top line
	Canvas->K2_DrawLine(
		FVector2D(CenterX, CenterY - Size),
		FVector2D(CenterX, CenterY - Gap),
		Thickness, Color);

	// Bottom line
	Canvas->K2_DrawLine(
		FVector2D(CenterX, CenterY + Gap),
		FVector2D(CenterX, CenterY + Size),
		Thickness, Color);

	// Left line
	Canvas->K2_DrawLine(
		FVector2D(CenterX - Size, CenterY),
		FVector2D(CenterX - Gap, CenterY),
		Thickness, Color);

	// Right line
	Canvas->K2_DrawLine(
		FVector2D(CenterX + Gap, CenterY),
		FVector2D(CenterX + Size, CenterY),
		Thickness, Color);
}

// ==================== HUD Text Drawing ====================

void AFighterHUD::DrawSettingsInfo(AFighterPawn* Fighter)
{
	if (!HUDFont || !Fighter) return;

	float CanvasWidth = Canvas->SizeX;
	float CanvasHeight = Canvas->SizeY;

	// Position at bottom-right
	float PanelWidth = 300.0f;
	float PanelHeight = LineSpacing * 2.0f + 16.0f;
	float X = CanvasWidth - ScreenMargin - PanelWidth;
	float Y = CanvasHeight - ScreenMargin - PanelHeight;

	// Draw semi-transparent background panel
	FLinearColor PanelColor(0.0f, 0.0f, 0.0f, 0.4f);
	Canvas->K2_DrawBox(FVector2D(X - 4.0f, Y - 4.0f), FVector2D(PanelWidth + 8.0f, PanelHeight + 8.0f), 1.0f, PanelColor);

	// Sound Volume
	int32 VolPercent = FMath::RoundToInt(Fighter->GetSoundVolume() * 100.0f);
	FString VolText = FString::Printf(TEXT("Sound Volume: %d%%  [-/+]"), VolPercent);
	FCanvasTextItem VolItem(FVector2D(X, Y), FText::FromString(VolText), HUDFont, SettingsTextColor);
	VolItem.Scale = FVector2D(TextScale, TextScale);
	VolItem.bOutlined = true;
	VolItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(VolItem);

	Y += LineSpacing;

	// Mouse Sensitivity
	FString SensText = FString::Printf(TEXT("Mouse Sensitivity: %.1f  [</>]"), Fighter->GetAimSensitivity());
	FCanvasTextItem SensItem(FVector2D(X, Y), FText::FromString(SensText), HUDFont, SettingsTextColor);
	SensItem.Scale = FVector2D(TextScale, TextScale);
	SensItem.bOutlined = true;
	SensItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(SensItem);
}

void AFighterHUD::DrawScoreInfo(AFighterPawn* Fighter)
{
	if (!HUDFont || !Fighter) return;

	float CanvasWidth = Canvas->SizeX;

	// Build text lines
	FString TankText = FString::Printf(TEXT("Tanks: %d"), Fighter->GetTanksDestroyed());
	FString HeliText = FString::Printf(TEXT("Helis: %d"), Fighter->GetHelisDestroyed());
	int32 TotalKills = Fighter->GetTanksDestroyed() + Fighter->GetHelisDestroyed();
	FString TotalText = FString::Printf(TEXT("Total Kills: %d"), TotalKills);
	FString AltText = FString::Printf(TEXT("Altitude: %.0f ft"), Fighter->GetCurrentAltitude());
	FString SpeedText = FString::Printf(TEXT("Speed: %.0f"), Fighter->GetCurrentSpeed());

	// Calculate right-aligned X position
	// Estimate max text width (use the longest line)
	float PanelWidth = 240.0f;
	float X = CanvasWidth - ScreenMargin - PanelWidth;
	float Y = ScreenMargin;

	// Draw semi-transparent background panel
	float PanelHeight = LineSpacing * 5.0f + 16.0f;
	FLinearColor PanelColor(0.0f, 0.0f, 0.0f, 0.4f);
	Canvas->K2_DrawBox(FVector2D(X - 4.0f, Y - 4.0f), FVector2D(PanelWidth + 8.0f, PanelHeight), 1.0f, PanelColor);

	// Total Kills (highlighted)
	FCanvasTextItem TotalItem(FVector2D(X, Y), FText::FromString(TotalText), HUDFont, ScoreTextColor);
	TotalItem.Scale = FVector2D(TextScale, TextScale);
	TotalItem.bOutlined = true;
	TotalItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(TotalItem);
	Y += LineSpacing;

	// Tanks
	FCanvasTextItem TankItem(FVector2D(X, Y), FText::FromString(TankText), HUDFont, ScoreTextColor);
	TankItem.Scale = FVector2D(TextScale, TextScale);
	TankItem.bOutlined = true;
	TankItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(TankItem);
	Y += LineSpacing;

	// Helis
	FCanvasTextItem HeliItem(FVector2D(X, Y), FText::FromString(HeliText), HUDFont, ScoreTextColor);
	HeliItem.Scale = FVector2D(TextScale, TextScale);
	HeliItem.bOutlined = true;
	HeliItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(HeliItem);
	Y += LineSpacing;

	// Altitude
	FCanvasTextItem AltItem(FVector2D(X, Y), FText::FromString(AltText), HUDFont, AltitudeTextColor);
	AltItem.Scale = FVector2D(TextScale, TextScale);
	AltItem.bOutlined = true;
	AltItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(AltItem);
	Y += LineSpacing;

	// Speed
	FCanvasTextItem SpeedItem(FVector2D(X, Y), FText::FromString(SpeedText), HUDFont, AltitudeTextColor);
	SpeedItem.Scale = FVector2D(TextScale, TextScale);
	SpeedItem.bOutlined = true;
	SpeedItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(SpeedItem);
}
