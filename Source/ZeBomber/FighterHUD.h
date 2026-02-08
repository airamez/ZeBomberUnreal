// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FighterHUD.generated.h"

/**
 * HUD class for the Fighter pawn.
 * Draws two crosshairs:
 *   1. RED crosshair  - predicted bomb impact point (follows airplane movement)
 *   2. WHITE crosshair - mouse-aimed rocket crosshair (follows mouse cursor)
 */
UCLASS()
class ZEBOMBER_API AFighterHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFighterHUD();

	virtual void DrawHUD() override;

protected:
	// ==================== Bomb Crosshair (Red) ====================

	/** Size of the bomb crosshair circle (screen pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|BombCrosshair")
	float BombCrosshairRadius = 20.0f;

	/** Thickness of the bomb crosshair lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|BombCrosshair")
	float BombCrosshairThickness = 2.0f;

	/** Color of the bomb crosshair */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|BombCrosshair")
	FLinearColor BombCrosshairColor = FLinearColor(1.0f, 0.0f, 0.0f, 0.9f);

	/** Number of segments used to draw the bomb crosshair circle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|BombCrosshair")
	int32 BombCircleSegments = 24;

	// ==================== Rocket Crosshair (White) ====================

	/** Half-length of the rocket crosshair lines (screen pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|RocketCrosshair")
	float RocketCrosshairSize = 14.0f;

	/** Thickness of the rocket crosshair lines */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|RocketCrosshair")
	float RocketCrosshairThickness = 2.0f;

	/** Gap in the center of the rocket crosshair (pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|RocketCrosshair")
	float RocketCrosshairGap = 4.0f;

	/** Color of the rocket crosshair */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|RocketCrosshair")
	FLinearColor RocketCrosshairColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.95f);

	/** Dot radius at the center of the rocket crosshair */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|RocketCrosshair")
	float RocketCenterDotRadius = 2.0f;

private:
	/** Draw a circle on the HUD canvas */
	void DrawCircle(float CenterX, float CenterY, float Radius, int32 Segments, FLinearColor Color, float Thickness);

	/** Draw a + shaped crosshair with a center gap */
	void DrawCrosshairPlus(float CenterX, float CenterY, float Size, float Gap, FLinearColor Color, float Thickness);
};
