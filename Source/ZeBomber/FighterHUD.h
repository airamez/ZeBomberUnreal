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

	// ==================== HUD Text ====================

	/** Color for settings text (bottom-right) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Text")
	FLinearColor SettingsTextColor = FLinearColor(0.8f, 0.8f, 0.8f, 0.85f);

	/** Color for score text (top-left) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Text")
	FLinearColor ScoreTextColor = FLinearColor(0.2f, 1.0f, 0.4f, 0.9f);

	/** Color for altitude text */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Text")
	FLinearColor AltitudeTextColor = FLinearColor(0.4f, 0.8f, 1.0f, 0.9f);

	/** Text scale for HUD info */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Text", meta = (ClampMin = "0.5"))
	float TextScale = 1.2f;

	/** Margin from screen edges (pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Text", meta = (ClampMin = "0.0"))
	float ScreenMargin = 20.0f;

	/** Line spacing between text rows (pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Text", meta = (ClampMin = "0.0"))
	float LineSpacing = 24.0f;

	// ==================== Jet HUD (Pitch Ladder / Horizon) ====================

	/** Color of the jet HUD overlay (classic yellow-green) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|JetHUD")
	FLinearColor JetHUDColor = FLinearColor(0.0f, 1.0f, 0.2f, 0.7f);

	/** Dimmer color for secondary elements */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|JetHUD")
	FLinearColor JetHUDDimColor = FLinearColor(0.0f, 1.0f, 0.2f, 0.35f);

	/** Line thickness for jet HUD elements */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|JetHUD", meta = (ClampMin = "0.5"))
	float JetHUDThickness = 1.5f;

	/** Width of the horizon line (pixels from center) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|JetHUD", meta = (ClampMin = "20.0"))
	float HorizonLineWidth = 100.0f;

	/** Width of pitch ladder lines (pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|JetHUD", meta = (ClampMin = "10.0"))
	float PitchLadderWidth = 60.0f;

	/** Pixels per degree for pitch ladder spacing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|JetHUD", meta = (ClampMin = "1.0"))
	float PitchPixelsPerDegree = 4.0f;

	/** Pitch ladder step in degrees (a line every N degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|JetHUD", meta = (ClampMin = "1"))
	int32 PitchLadderStep = 10;

	/** Max pitch degrees shown on ladder above/below horizon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|JetHUD", meta = (ClampMin = "10"))
	int32 PitchLadderRange = 20;

	// ==================== Radar ====================

	/** Radar display radius on screen (pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar", meta = (ClampMin = "30.0"))
	float RadarRadius = 90.0f;

	/** World range the radar covers (units) — enemies beyond this won't show */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar", meta = (ClampMin = "500.0"))
	float RadarWorldRange = 15000.0f;

	/** Radar background color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar")
	FLinearColor RadarBgColor = FLinearColor(0.0f, 0.05f, 0.1f, 0.6f);

	/** Radar ring/border color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar")
	FLinearColor RadarRingColor = FLinearColor(0.0f, 0.8f, 1.0f, 0.5f);

	/** Tank dot color (red) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar")
	FLinearColor RadarTankColor = FLinearColor(1.0f, 0.15f, 0.15f, 1.0f);

	/** Heli dot color (yellow) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar")
	FLinearColor RadarHeliColor = FLinearColor(1.0f, 0.9f, 0.1f, 1.0f);

	/** Dot size for enemies on radar (pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar", meta = (ClampMin = "1.0"))
	float RadarDotSize = 4.0f;

	/** Height bar width for helis (pixels) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar", meta = (ClampMin = "1.0"))
	float RadarHeliBarWidth = 2.0f;

	/** Max height bar length (pixels) for max heli altitude */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar", meta = (ClampMin = "2.0"))
	float RadarHeliBarMaxLength = 20.0f;

	/** Max heli altitude for bar scaling (units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Radar", meta = (ClampMin = "100.0"))
	float RadarHeliMaxAltitude = 1000.0f;

private:
	/** Draw a circle on the HUD canvas */
	void DrawCircle(float CenterX, float CenterY, float Radius, int32 Segments, FLinearColor Color, float Thickness);

	/** Draw a + shaped crosshair with a center gap */
	void DrawCrosshairPlus(float CenterX, float CenterY, float Size, float Gap, FLinearColor Color, float Thickness);

	/** Draw settings info (bottom-right) */
	void DrawSettingsInfo(class AFighterPawn* Fighter);

	/** Draw score and altitude (top-left) */
	void DrawScoreInfo(class AFighterPawn* Fighter);

	/** Draw radar display (top-right) */
	void DrawRadar(class AFighterPawn* Fighter);

	/** Draw jet fighter HUD overlay (horizon line, pitch ladder, heading, speed/alt) */
	void DrawJetHUD(class AFighterPawn* Fighter);

	/** Draw a filled circle on the HUD canvas */
	void DrawFilledCircle(float CenterX, float CenterY, float Radius, int32 Segments, FLinearColor Color);

	/** Cached HUD font */
	UFont* HUDFont = nullptr;
};
