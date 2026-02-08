// Copyright Epic Games, Inc. All Rights Reserved.

#include "FighterHUD.h"
#include "FighterPawn.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"

AFighterHUD::AFighterHUD()
{
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

	// ==================== WHITE Rocket Aim Crosshair (at mouse position) ====================
	{
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			DrawCrosshairPlus(MouseX, MouseY, RocketCrosshairSize, RocketCrosshairGap, RocketCrosshairColor, RocketCrosshairThickness);

			// Center dot
			if (RocketCenterDotRadius > 0.0f)
			{
				DrawCircle(MouseX, MouseY, RocketCenterDotRadius, 8, RocketCrosshairColor, RocketCrosshairThickness);
			}
		}
	}
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
