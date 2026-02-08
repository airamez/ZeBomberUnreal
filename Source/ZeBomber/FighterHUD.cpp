// Copyright Epic Games, Inc. All Rights Reserved.

#include "FighterHUD.h"
#include "FighterPawn.h"
#include "TankAI.h"
#include "HeliAI.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

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

	// ==================== HUD Text & Radar ====================
	DrawSettingsInfo(Fighter);
	DrawScoreInfo(Fighter);
	DrawRadar(Fighter);
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

	// Position at top-left
	float X = ScreenMargin;
	float Y = ScreenMargin;

	// Build text lines
	FString TankText = FString::Printf(TEXT("Tanks: %d"), Fighter->GetTanksDestroyed());
	FString HeliText = FString::Printf(TEXT("Helis: %d"), Fighter->GetHelisDestroyed());
	int32 TotalKills = Fighter->GetTanksDestroyed() + Fighter->GetHelisDestroyed();
	FString TotalText = FString::Printf(TEXT("Total Kills: %d"), TotalKills);
	FString AltText = FString::Printf(TEXT("Altitude: %.0f ft"), Fighter->GetCurrentAltitude());
	FString SpeedText = FString::Printf(TEXT("Speed: %.0f"), Fighter->GetCurrentSpeed());

	// Draw semi-transparent background panel
	float PanelWidth = 240.0f;
	float PanelHeight = LineSpacing * 5.0f + 16.0f;
	FLinearColor PanelColor(0.0f, 0.0f, 0.0f, 0.4f);
	Canvas->K2_DrawBox(FVector2D(X - 4.0f, Y - 4.0f), FVector2D(PanelWidth, PanelHeight), 1.0f, PanelColor);

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

// ==================== Radar ====================

void AFighterHUD::DrawFilledCircle(float CenterX, float CenterY, float Radius, int32 Segments, FLinearColor Color)
{
	if (Segments < 3) Segments = 3;

	// Draw filled circle using triangle fan (line-based approximation with thick lines)
	float AngleStep = 2.0f * PI / static_cast<float>(Segments);
	for (int32 i = 0; i < Segments; ++i)
	{
		float Angle1 = AngleStep * i;
		float Angle2 = AngleStep * (i + 1);

		FVector2D P1(CenterX + Radius * FMath::Cos(Angle1), CenterY + Radius * FMath::Sin(Angle1));
		FVector2D P2(CenterX + Radius * FMath::Cos(Angle2), CenterY + Radius * FMath::Sin(Angle2));

		// Draw triangle from center to edge segment
		Canvas->K2_DrawLine(FVector2D(CenterX, CenterY), P1, Radius * 0.5f, Color);
		Canvas->K2_DrawLine(P1, P2, 1.0f, Color);
	}
}

void AFighterHUD::DrawRadar(AFighterPawn* Fighter)
{
	if (!Fighter || !Canvas) return;

	UWorld* World = GetWorld();
	if (!World) return;

	float CanvasWidth = Canvas->SizeX;

	// Radar center position (top-right)
	float RadarCX = CanvasWidth - ScreenMargin - RadarRadius - 10.0f;
	float RadarCY = ScreenMargin + RadarRadius + 10.0f;

	// --- Draw radar background (filled square) ---
	float L = RadarCX - RadarRadius;
	float R = RadarCX + RadarRadius;
	float T = RadarCY - RadarRadius;
	float B = RadarCY + RadarRadius;
	Canvas->K2_DrawBox(FVector2D(L, T), FVector2D(R - L, B - T), 1.0f, RadarBgColor);
	Canvas->K2_DrawLine(FVector2D(L, T), FVector2D(R, T), 1.5f, RadarRingColor); // top
	Canvas->K2_DrawLine(FVector2D(R, T), FVector2D(R, B), 1.5f, RadarRingColor); // right
	Canvas->K2_DrawLine(FVector2D(R, B), FVector2D(L, B), 1.5f, RadarRingColor); // bottom
	Canvas->K2_DrawLine(FVector2D(L, B), FVector2D(L, T), 1.5f, RadarRingColor); // left

	// --- Draw inner concentric rings ---
	DrawCircle(RadarCX, RadarCY, RadarRadius * 0.66f, 36, FLinearColor(RadarRingColor.R, RadarRingColor.G, RadarRingColor.B, RadarRingColor.A * 0.4f), 1.0f);
	DrawCircle(RadarCX, RadarCY, RadarRadius * 0.33f, 24, FLinearColor(RadarRingColor.R, RadarRingColor.G, RadarRingColor.B, RadarRingColor.A * 0.3f), 1.0f);

	// --- Draw cross lines (N/S/E/W) ---
	FLinearColor CrossColor(RadarRingColor.R, RadarRingColor.G, RadarRingColor.B, RadarRingColor.A * 0.3f);
	Canvas->K2_DrawLine(FVector2D(RadarCX - RadarRadius, RadarCY), FVector2D(RadarCX + RadarRadius, RadarCY), 1.0f, CrossColor);
	Canvas->K2_DrawLine(FVector2D(RadarCX, RadarCY - RadarRadius), FVector2D(RadarCX, RadarCY + RadarRadius), 1.0f, CrossColor);

	// --- Draw player triangle at center ---
	{
		float TriSize = 5.0f;
		FLinearColor PlayerColor(0.0f, 1.0f, 0.5f, 1.0f);
		FVector2D Top(RadarCX, RadarCY - TriSize);
		FVector2D BotL(RadarCX - TriSize * 0.7f, RadarCY + TriSize * 0.6f);
		FVector2D BotR(RadarCX + TriSize * 0.7f, RadarCY + TriSize * 0.6f);
		Canvas->K2_DrawLine(Top, BotL, 2.0f, PlayerColor);
		Canvas->K2_DrawLine(BotL, BotR, 2.0f, PlayerColor);
		Canvas->K2_DrawLine(BotR, Top, 2.0f, PlayerColor);
	}

	// --- Get player position and yaw for relative positioning ---
	FVector PlayerPos = Fighter->GetActorLocation();
	float PlayerYawRad = FMath::DegreesToRadians(Fighter->GetActorRotation().Yaw);

	// Scale factor: world units -> radar pixels (apply zoom from FighterPawn)
	float EffectiveRange = RadarWorldRange * Fighter->GetRadarZoom();
	float Scale = RadarRadius / EffectiveRange;

	// --- Draw tanks (red dots) ---
	for (TActorIterator<ATankAI> It(World); It; ++It)
	{
		ATankAI* Tank = *It;
		if (!Tank) continue;

		FVector RelPos = Tank->GetActorLocation() - PlayerPos;

		// Rotate relative to player's yaw (so forward is always up on radar)
		float RotX = RelPos.X * FMath::Cos(-PlayerYawRad) - RelPos.Y * FMath::Sin(-PlayerYawRad);
		float RotY = RelPos.X * FMath::Sin(-PlayerYawRad) + RelPos.Y * FMath::Cos(-PlayerYawRad);

		// Map to radar space (UE: X=forward, Y=right -> Radar: up=-Y screen, right=+X screen)
		float DotX = RadarCX + RotY * Scale;
		float DotY = RadarCY - RotX * Scale;

		// Clamp to radar square
		float Margin = RadarDotSize + 1.0f;
		DotX = FMath::Clamp(DotX, L + Margin, R - Margin);
		DotY = FMath::Clamp(DotY, T + Margin, B - Margin);

		// Draw red filled dot (diamond shape for tanks)
		float S = RadarDotSize;
		Canvas->K2_DrawLine(FVector2D(DotX, DotY - S), FVector2D(DotX + S, DotY), 2.0f, RadarTankColor);
		Canvas->K2_DrawLine(FVector2D(DotX + S, DotY), FVector2D(DotX, DotY + S), 2.0f, RadarTankColor);
		Canvas->K2_DrawLine(FVector2D(DotX, DotY + S), FVector2D(DotX - S, DotY), 2.0f, RadarTankColor);
		Canvas->K2_DrawLine(FVector2D(DotX - S, DotY), FVector2D(DotX, DotY - S), 2.0f, RadarTankColor);
	}

	// --- Draw helicopters (yellow dots with height bar) ---
	for (TActorIterator<AHeliAI> It(World); It; ++It)
	{
		AHeliAI* Heli = *It;
		if (!Heli) continue;

		FVector HeliPos = Heli->GetActorLocation();
		FVector RelPos = HeliPos - PlayerPos;

		// Rotate relative to player's yaw
		float RotX = RelPos.X * FMath::Cos(-PlayerYawRad) - RelPos.Y * FMath::Sin(-PlayerYawRad);
		float RotY = RelPos.X * FMath::Sin(-PlayerYawRad) + RelPos.Y * FMath::Cos(-PlayerYawRad);

		// Map to radar space
		float DotX = RadarCX + RotY * Scale;
		float DotY = RadarCY - RotX * Scale;

		// Clamp to radar square
		float Margin = RadarDotSize + 1.0f;
		DotX = FMath::Clamp(DotX, L + Margin, R - Margin);
		DotY = FMath::Clamp(DotY, T + Margin, B - Margin);

		// Draw yellow circle dot for helis
		DrawCircle(DotX, DotY, RadarDotSize, 8, RadarHeliColor, 2.0f);

		// Draw vertical height bar below the dot
		float HeliAlt = FMath::Max(0.0f, HeliPos.Z);
		float BarLength = FMath::Clamp(HeliAlt / RadarHeliMaxAltitude, 0.0f, 1.0f) * RadarHeliBarMaxLength;
		if (BarLength > 1.0f)
		{
			Canvas->K2_DrawLine(
				FVector2D(DotX, DotY + RadarDotSize + 1.0f),
				FVector2D(DotX, DotY + RadarDotSize + 1.0f + BarLength),
				RadarHeliBarWidth, RadarHeliColor);
		}
	}

	// --- Draw "RADAR" label ---
	if (HUDFont)
	{
		FString RadarLabel = TEXT("RADAR");
		FCanvasTextItem LabelItem(FVector2D(RadarCX - 18.0f, RadarCY + RadarRadius + 4.0f), FText::FromString(RadarLabel), HUDFont, RadarRingColor);
		LabelItem.Scale = FVector2D(0.8f, 0.8f);
		LabelItem.bOutlined = true;
		LabelItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
		Canvas->DrawItem(LabelItem);
	}
}
