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
		InstructionsFont = FontObj.Object;
	}
}

void AFighterHUD::BeginPlay()
{
	Super::BeginPlay();
}

void AFighterHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	AFighterPawn* Fighter = Cast<AFighterPawn>(PC->GetPawn());
	if (!Fighter) return;

	EGameState State = Fighter->GetGameState();

	// Draw game state screens (instructions, pause, game over, wave end)
	if (State != EGameState::Playing)
	{
		DrawGameScreen(Fighter);
		return;
	}

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

	// ==================== Jet HUD Overlay ====================
	DrawJetHUD(Fighter);

	// ==================== Speed & Altitude (center) ====================
	DrawSpeedAltitude(Fighter);

	// ==================== HUD Text & Radar ====================
	DrawSettingsInfo(Fighter);
	DrawScoreInfo(Fighter);
	DrawRadar(Fighter);

	// ==================== Damage Flash ====================
	DrawDamageFlash(Fighter);
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

	// Build text strings first so we can measure them
	int32 VolPercent = FMath::RoundToInt(Fighter->GetSoundVolume() * 100.0f);
	FString VolText = FString::Printf(TEXT("Volume: %d%%"), VolPercent);
	FString SensText = FString::Printf(TEXT("Sensitivity: %.1f"), Fighter->GetAimSensitivity());
	
	// Add FPS text if enabled
	FString FpsText;
	float FpsWidth = 0.0f;
	if (Fighter->IsFpsDisplayEnabled())
	{
		// Get smoothed FPS from FighterPawn
		float FPS = Fighter->GetCurrentFps();
		FpsText = FString::Printf(TEXT("FPS: %.0f"), FPS);
		FpsWidth = HUDFont->GetStringSize(*FpsText) * TextScale;
	}

	// Measure text to auto-size the panel
	float VolWidth = HUDFont->GetStringSize(*VolText) * TextScale;
	float SensWidth = HUDFont->GetStringSize(*SensText) * TextScale;
	float MaxTextWidth = FMath::Max3(VolWidth, SensWidth, FpsWidth);

	// Position at lower-right corner, tight fit
	float Padding = 6.0f;
	float LineCount = Fighter->IsFpsDisplayEnabled() ? 3.0f : 2.0f;
	float PanelWidth = MaxTextWidth + Padding * 2.0f;
	float PanelHeight = LineSpacing * LineCount + Padding * 2.0f;
	float Margin = 8.0f;
	float X = CanvasWidth - Margin - PanelWidth;
	float Y = CanvasHeight - Margin - PanelHeight;

	// Draw tight background panel
	FLinearColor PanelColor(0.0f, 0.0f, 0.0f, 0.5f);
	Canvas->K2_DrawBox(FVector2D(X, Y), FVector2D(PanelWidth, PanelHeight), 1.0f, PanelColor);

	float RightEdge = X + PanelWidth - Padding;
	float TextY = Y + Padding;

	// Sound Volume (right-aligned)
	FCanvasTextItem VolItem(FVector2D(RightEdge - VolWidth, TextY), FText::FromString(VolText), HUDFont, SettingsTextColor);
	VolItem.Scale = FVector2D(TextScale, TextScale);
	VolItem.bOutlined = true;
	VolItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(VolItem);

	TextY += LineSpacing;

	// Mouse Sensitivity (right-aligned)
	FCanvasTextItem SensItem(FVector2D(RightEdge - SensWidth, TextY), FText::FromString(SensText), HUDFont, SettingsTextColor);
	SensItem.Scale = FVector2D(TextScale, TextScale);
	SensItem.bOutlined = true;
	SensItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(SensItem);

	// FPS display (if enabled)
	if (Fighter->IsFpsDisplayEnabled())
	{
		TextY += LineSpacing;
		FCanvasTextItem FpsItem(FVector2D(RightEdge - FpsWidth, TextY), FText::FromString(FpsText), HUDFont, SettingsTextColor);
		FpsItem.Scale = FVector2D(TextScale, TextScale);
		FpsItem.bOutlined = true;
		FpsItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
		Canvas->DrawItem(FpsItem);
	}
}

void AFighterHUD::DrawScoreInfo(AFighterPawn* Fighter)
{
	if (!HUDFont || !Fighter) return;

	// Position at top-left
	float X = ScreenMargin;
	float Y = ScreenMargin;

	// Build text lines
	FString WaveText = FString::Printf(TEXT("Wave: %d"), Fighter->GetCurrentWave());
	FString TankText = FString::Printf(TEXT("Tanks: %d/%d"), Fighter->GetTanksDestroyed(), Fighter->GetWaveTotalTanks());
	FString HeliText = FString::Printf(TEXT("Helis: %d/%d"), Fighter->GetHelisDestroyed(), Fighter->GetWaveTotalHelis());
	FString HPText = FString::Printf(TEXT("Base HP: %d/%d"), Fighter->GetBaseHP(), Fighter->GetBaseMaxHP());

	// Draw semi-transparent background panel
	float PanelWidth = 140.0f;
	float PanelHeight = LineSpacing * 4.0f + 16.0f;
	FLinearColor PanelColor(0.0f, 0.0f, 0.0f, 0.4f);
	Canvas->K2_DrawBox(FVector2D(X - 4.0f, Y - 4.0f), FVector2D(PanelWidth, PanelHeight), 1.0f, PanelColor);

	// Base HP (highlighted, red if low)
	FLinearColor HPColor = (Fighter->GetBaseHP() < Fighter->GetBaseMaxHP() / 4) ? FLinearColor(1.0f, 0.2f, 0.2f, 1.0f) : ScoreTextColor;
	FCanvasTextItem HPItem(FVector2D(X, Y), FText::FromString(HPText), HUDFont, HPColor);
	HPItem.Scale = FVector2D(TextScale, TextScale);
	HPItem.bOutlined = true;
	HPItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(HPItem);
	Y += LineSpacing;

	// Wave
	FCanvasTextItem WaveItem(FVector2D(X, Y), FText::FromString(WaveText), HUDFont, ScoreTextColor);
	WaveItem.Scale = FVector2D(TextScale, TextScale);
	WaveItem.bOutlined = true;
	WaveItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(WaveItem);
	Y += LineSpacing;

	// Tanks X/Y
	FCanvasTextItem TankItem(FVector2D(X, Y), FText::FromString(TankText), HUDFont, ScoreTextColor);
	TankItem.Scale = FVector2D(TextScale, TextScale);
	TankItem.bOutlined = true;
	TankItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(TankItem);
	Y += LineSpacing;

	// Helis X/Y
	FCanvasTextItem HeliItem(FVector2D(X, Y), FText::FromString(HeliText), HUDFont, ScoreTextColor);
	HeliItem.Scale = FVector2D(TextScale, TextScale);
	HeliItem.bOutlined = true;
	HeliItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(HeliItem);
	Y += LineSpacing;

}

// ==================== Speed & Altitude (center screen) ====================

void AFighterHUD::DrawSpeedAltitude(AFighterPawn* Fighter)
{
	if (!HUDFont || !Canvas || !Fighter) return;

	float CX = Canvas->SizeX * 0.5f;
	float CY = Canvas->SizeY * 0.5f;

	FLinearColor SpeedColor(0.3f, 1.0f, 0.5f, 0.9f);
	FLinearColor AltColor(0.3f, 1.0f, 0.5f, 0.9f); // Same as speed
	FLinearColor LabelColor(0.7f, 0.7f, 0.7f, 0.7f);
	float ValueScale = 1.6f;
	float LabelScale = 0.85f;

	// Speed on the left side of center
	float SpeedX = CX - 200.0f;
	FString SpeedVal = FString::Printf(TEXT("%.0f"), Fighter->GetCurrentSpeed());
	FCanvasTextItem SpeedItem(FVector2D(SpeedX, CY - 12.0f), FText::FromString(SpeedVal), HUDFont, SpeedColor);
	SpeedItem.Scale = FVector2D(ValueScale, ValueScale);
	SpeedItem.bOutlined = true;
	SpeedItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
	Canvas->DrawItem(SpeedItem);

	FCanvasTextItem SpeedLabel(FVector2D(SpeedX, CY + 18.0f), FText::FromString(TEXT("SPD")), HUDFont, LabelColor);
	SpeedLabel.Scale = FVector2D(LabelScale, LabelScale);
	SpeedLabel.bOutlined = true;
	SpeedLabel.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.4f);
	Canvas->DrawItem(SpeedLabel);

	// Altitude on the right side of center
	float AltX = CX + 150.0f;
	FString AltVal = FString::Printf(TEXT("%.0f"), Fighter->GetCurrentAltitude());
	FCanvasTextItem AltItem(FVector2D(AltX, CY - 12.0f), FText::FromString(AltVal), HUDFont, AltColor);
	AltItem.Scale = FVector2D(ValueScale, ValueScale);
	AltItem.bOutlined = true;
	AltItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);
	Canvas->DrawItem(AltItem);

	FCanvasTextItem AltLabel(FVector2D(AltX, CY + 18.0f), FText::FromString(TEXT("ALT")), HUDFont, LabelColor);
	AltLabel.Scale = FVector2D(LabelScale, LabelScale);
	AltLabel.bOutlined = true;
	AltLabel.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.4f);
	Canvas->DrawItem(AltLabel);
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

// ==================== Jet Fighter HUD Overlay ====================

void AFighterHUD::DrawJetHUD(AFighterPawn* Fighter)
{
	if (!Canvas || !Fighter || !HUDFont) return;

	float CX = Canvas->SizeX * 0.5f;
	float CY = Canvas->SizeY * 0.5f;

	FRotator Rot = Fighter->GetActorRotation();
	float PitchDeg = Rot.Pitch;
	float RollRad = FMath::DegreesToRadians(Rot.Roll);
	float YawDeg = Rot.Yaw;
	if (YawDeg < 0.0f) YawDeg += 360.0f;

	float CosR = FMath::Cos(RollRad);
	float SinR = FMath::Sin(RollRad);

	// Helper lambda: rotate a point around screen center by roll angle
	auto RotatePoint = [&](float X, float Y) -> FVector2D
	{
		float RX = X * CosR - Y * SinR;
		float RY = X * SinR + Y * CosR;
		return FVector2D(CX + RX, CY + RY);
	};

	float HW = HorizonLineWidth;
	float LadderHalfW = PitchLadderWidth * 0.5f;
	float GapHalfW = 15.0f;

	// ---- Horizon Line ----
	float HorizonOffset = PitchDeg * PitchPixelsPerDegree;
	FVector2D HL = RotatePoint(-HW, HorizonOffset);
	FVector2D HR = RotatePoint(HW, HorizonOffset);
	Canvas->K2_DrawLine(HL, HR, JetHUDThickness + 0.5f, JetHUDColor);

	// Small ticks at horizon ends
	FVector2D HLT = RotatePoint(-HW, HorizonOffset - 6.0f);
	FVector2D HRT = RotatePoint(HW, HorizonOffset - 6.0f);
	Canvas->K2_DrawLine(HL, HLT, JetHUDThickness, JetHUDColor);
	Canvas->K2_DrawLine(HR, HRT, JetHUDThickness, JetHUDColor);

	// ---- Pitch Ladder (only ±10 and ±20 = 4 lines total) ----
	for (int32 Deg = -PitchLadderRange; Deg <= PitchLadderRange; Deg += PitchLadderStep)
	{
		if (Deg == 0) continue;

		float YOff = (PitchDeg - static_cast<float>(Deg)) * PitchPixelsPerDegree;
		if (FMath::Abs(YOff) > CY * 0.7f) continue;

		bool bNeg = (Deg < 0);
		FLinearColor LineColor = bNeg ? JetHUDDimColor : JetHUDColor;

		if (bNeg)
		{
			// Dashed line for below horizon
			float SegLen = (LadderHalfW - GapHalfW) * 0.4f;
			float SegGap = (LadderHalfW - GapHalfW) * 0.2f;
			for (int32 d = 0; d < 2; d++)
			{
				float S = GapHalfW + d * (SegLen + SegGap);
				float E = S + SegLen;
				Canvas->K2_DrawLine(RotatePoint(-E, YOff), RotatePoint(-S, YOff), JetHUDThickness, LineColor);
				Canvas->K2_DrawLine(RotatePoint(S, YOff), RotatePoint(E, YOff), JetHUDThickness, LineColor);
			}
		}
		else
		{
			// Solid line for above horizon (with center gap)
			Canvas->K2_DrawLine(RotatePoint(-LadderHalfW, YOff), RotatePoint(-GapHalfW, YOff), JetHUDThickness, LineColor);
			Canvas->K2_DrawLine(RotatePoint(GapHalfW, YOff), RotatePoint(LadderHalfW, YOff), JetHUDThickness, LineColor);
		}

		// Small end ticks (up for positive, down for negative)
		float TickDir = bNeg ? 4.0f : -4.0f;
		Canvas->K2_DrawLine(RotatePoint(-LadderHalfW, YOff), RotatePoint(-LadderHalfW, YOff + TickDir), JetHUDThickness, LineColor);
		Canvas->K2_DrawLine(RotatePoint(LadderHalfW, YOff), RotatePoint(LadderHalfW, YOff + TickDir), JetHUDThickness, LineColor);

		// Degree label (right side only, compact)
		FString DegStr = FString::Printf(TEXT("%d"), Deg);
		FVector2D LabelPos = RotatePoint(LadderHalfW + 5.0f, YOff - 5.0f);
		FCanvasTextItem Label(LabelPos, FText::FromString(DegStr), HUDFont, LineColor);
		Label.Scale = FVector2D(0.55f, 0.55f);
		Label.bOutlined = true;
		Label.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.4f);
		Canvas->DrawItem(Label);
	}

	// ---- Aircraft Reference Symbol (fixed at center) ----
	float WingW = 18.0f;
	float WingGap = 6.0f;

	Canvas->K2_DrawLine(FVector2D(CX - WingGap, CY), FVector2D(CX - WingGap - WingW, CY), JetHUDThickness + 0.5f, JetHUDColor);
	Canvas->K2_DrawLine(FVector2D(CX - WingGap - WingW, CY), FVector2D(CX - WingGap - WingW, CY + 4.0f), JetHUDThickness + 0.5f, JetHUDColor);
	Canvas->K2_DrawLine(FVector2D(CX + WingGap, CY), FVector2D(CX + WingGap + WingW, CY), JetHUDThickness + 0.5f, JetHUDColor);
	Canvas->K2_DrawLine(FVector2D(CX + WingGap + WingW, CY), FVector2D(CX + WingGap + WingW, CY + 4.0f), JetHUDThickness + 0.5f, JetHUDColor);
	DrawCircle(CX, CY, 2.0f, 6, JetHUDColor, JetHUDThickness);

	// ---- Compact Heading Text (just above the ladder) ----
	{
		int32 Heading = FMath::RoundToInt32(YawDeg) % 360;
		if (Heading < 0) Heading += 360;
		FString HdgStr = FString::Printf(TEXT("HDG %03d"), Heading);
		float TextX = CX - 22.0f;
		float TextY = CY - HW * 0.6f - 14.0f;
		FCanvasTextItem HdgTxt(FVector2D(TextX, TextY), FText::FromString(HdgStr), HUDFont, JetHUDDimColor);
		HdgTxt.Scale = FVector2D(0.55f, 0.55f);
		HdgTxt.bOutlined = true;
		HdgTxt.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.4f);
		Canvas->DrawItem(HdgTxt);
	}

	// ---- Compact Speed & Altitude (left and right of ladder) ----
	{
		int32 SpeedKnots = FMath::RoundToInt32(Fighter->GetCurrentSpeed() * 0.0194384f);
		FString SpdStr = FString::Printf(TEXT("%d"), SpeedKnots);
		FCanvasTextItem SpdTxt(FVector2D(CX - HW - 40.0f, CY - 7.0f), FText::FromString(SpdStr), HUDFont, JetHUDColor);
		SpdTxt.Scale = FVector2D(0.65f, 0.65f);
		SpdTxt.bOutlined = true;
		SpdTxt.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.4f);
		Canvas->DrawItem(SpdTxt);

		int32 AltFeet = FMath::RoundToInt32(Fighter->GetActorLocation().Z * 0.0328084f);
		FString AltStr = FString::Printf(TEXT("%d"), AltFeet);
		FCanvasTextItem AltTxt(FVector2D(CX + HW + 10.0f, CY - 7.0f), FText::FromString(AltStr), HUDFont, JetHUDColor);
		AltTxt.Scale = FVector2D(0.65f, 0.65f);
		AltTxt.bOutlined = true;
		AltTxt.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.4f);
		Canvas->DrawItem(AltTxt);
	}
}

// ==================== Game Screen Overlays ====================

void AFighterHUD::DrawCenteredText(const FString& Text, float Y, FLinearColor Color, float Scale)
{
	if (!HUDFont || !Canvas) return;

	float CX = Canvas->SizeX * 0.5f;

	// Approximate text width for centering
	float CharW = 10.0f * Scale;
	float TextW = Text.Len() * CharW;

	FCanvasTextItem Item(FVector2D(CX - TextW * 0.5f, Y), FText::FromString(Text), HUDFont, Color);
	Item.Scale = FVector2D(Scale, Scale);
	Item.bOutlined = true;
	Item.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(Item);
}

void AFighterHUD::DrawLeftAlignedText(const FString& Text, float X, float Y, FLinearColor Color, float Scale)
{
	if (!HUDFont || !Canvas) return;

	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), HUDFont, Color);
	Item.Scale = FVector2D(Scale, Scale);
	Item.bOutlined = true;
	Item.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
	Canvas->DrawItem(Item);
}

void AFighterHUD::DrawDamageFlash(AFighterPawn* Fighter)
{
	if (!Canvas || !Fighter) return;

	float Alpha = Fighter->GetDamageFlashAlpha();
	if (Alpha <= 0.0f) return;

	FLinearColor FlashColor(1.0f, 0.0f, 0.0f, Alpha);
	FCanvasTileItem TileItem(FVector2D(0.0f, 0.0f), FVector2D(Canvas->SizeX, Canvas->SizeY), FlashColor);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
}

void AFighterHUD::DrawGameScreen(AFighterPawn* Fighter)
{
	if (!Canvas || !Fighter || !HUDFont) return;

	float CX = Canvas->SizeX * 0.5f;
	float CY = Canvas->SizeY * 0.5f;
	EGameState State = Fighter->GetGameState();

	// Dark overlay (filled rectangle covering entire screen)
	FLinearColor OverlayColor(0.0f, 0.0f, 0.0f, 0.85f);
	FCanvasTileItem OverlayTile(FVector2D(0.0f, 0.0f), FVector2D(Canvas->SizeX, Canvas->SizeY), OverlayColor);
	OverlayTile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(OverlayTile);

	FLinearColor TitleColor(1.0f, 0.9f, 0.2f, 1.0f);
	FLinearColor TextColor(0.9f, 0.9f, 0.9f, 0.9f);
	FLinearColor PromptColor(0.3f, 1.0f, 0.4f, 1.0f);
	FLinearColor RedColor(1.0f, 0.3f, 0.3f, 1.0f);

	// Left margin for instructions text (offset from center)
	float InstructionsX = CX - 250.0f;

	if (State == EGameState::Instructions)
	{
		DrawCenteredText(TEXT("ZEBOMBER"), CY - 220.0f, TitleColor, 2.5f);

		// Draw background panel for instructions
		TArray<FString> Lines;
		Fighter->GetInstructionsText().ParseIntoArrayLines(Lines);
		float PanelWidth = 520.0f;
		float PanelHeight = Lines.Num() * 20.0f + 40.0f;
		float PanelX = InstructionsX - 20.0f;
		float PanelY = CY - 170.0f;
		
		FLinearColor PanelColor(0.02f, 0.02f, 0.02f, 0.95f);
		FCanvasTileItem TileItem(FVector2D(PanelX, PanelY), FVector2D(PanelWidth, PanelHeight), PanelColor);
		TileItem.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(TileItem);

		// Split instructions text into lines and draw left-aligned with military styling
		float LineY = CY - 150.0f;
		for (const FString& Line : Lines)
		{
			FCanvasTextItem TextItem(FVector2D(InstructionsX, LineY), FText::FromString(Line), InstructionsFont, TextColor);
			TextItem.Scale = FVector2D(1.1f, 1.1f);
			TextItem.bOutlined = true;
			TextItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f);
			Canvas->DrawItem(TextItem);
			LineY += 18.0f; // Tighter line spacing
		}

		DrawCenteredText(StartMessage, CY + 180.0f, PromptColor, 1.3f);
	}
	else if (State == EGameState::Paused)
	{
		DrawCenteredText(TEXT("GAME PAUSED"), CY - 220.0f, TitleColor, 2.5f);

		// Draw background panel for instructions
		TArray<FString> Lines;
		Fighter->GetInstructionsText().ParseIntoArrayLines(Lines);
		float PanelWidth = 520.0f;
		float PanelHeight = Lines.Num() * 18.0f + 40.0f;
		float PanelX = InstructionsX - 20.0f;
		float PanelY = CY - 170.0f;
		
		FLinearColor PanelColor(0.02f, 0.02f, 0.02f, 0.95f);
		FCanvasTileItem TileItem(FVector2D(PanelX, PanelY), FVector2D(PanelWidth, PanelHeight), PanelColor);
		TileItem.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(TileItem);

		// Show instructions left-aligned with military styling
		float LineY = CY - 150.0f;
		for (const FString& Line : Lines)
		{
			FCanvasTextItem TextItem(FVector2D(InstructionsX, LineY), FText::FromString(Line), InstructionsFont, TextColor);
			TextItem.Scale = FVector2D(1.1f, 1.1f);
			TextItem.bOutlined = true;
			TextItem.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f);
			Canvas->DrawItem(TextItem);
			LineY += 18.0f; // Tighter line spacing
		}

		// Place resume/quit below instructions with some spacing
		float PromptY = LineY + 20.0f;
		DrawCenteredText(TEXT("Press C to Resume"), PromptY, PromptColor, 1.1f);
		DrawCenteredText(TEXT("Press X to Quit"), PromptY + 30.0f, RedColor, 1.1f);
	}
	else if (State == EGameState::GameOver)
	{
		DrawCenteredText(GameOverTitle, CY - 80.0f, RedColor, 3.0f);
		DrawCenteredText(GameOverSubtitle, CY - 20.0f, TextColor, 1.2f);

		// Stats
		FString StatsText = FString::Printf(TEXT("Waves survived: %d  |  Tanks: %d  |  Helis: %d"),
			Fighter->GetCurrentWave() > 0 ? Fighter->GetCurrentWave() - 1 : 0,
			Fighter->GetTanksDestroyed(), Fighter->GetHelisDestroyed());
		DrawCenteredText(StatsText, CY + 30.0f, TextColor, 0.9f);

		DrawCenteredText(GameOverRestartMessage, CY + 100.0f, PromptColor, 1.3f);
	}
	else if (State == EGameState::WaveEnd)
	{
		FString Title = FString::Printf(TEXT("WAVE %d COMPLETE!"), Fighter->GetCurrentWave());
		DrawCenteredText(Title, CY - 100.0f, TitleColor, 2.0f);

		FString TimeText = FString::Printf(TEXT("Time: %.1f seconds"), Fighter->GetWaveDuration());
		DrawCenteredText(TimeText, CY - 40.0f, TextColor, 1.1f);

		FString TankStats = FString::Printf(TEXT("Tanks destroyed: %d/%d"), Fighter->GetTanksDestroyed(), Fighter->GetWaveTotalTanks());
		FString HeliStats = FString::Printf(TEXT("Helis destroyed: %d/%d"), Fighter->GetHelisDestroyed(), Fighter->GetWaveTotalHelis());
		DrawCenteredText(TankStats, CY + 0.0f, TextColor, 1.0f);
		DrawCenteredText(HeliStats, CY + 28.0f, TextColor, 1.0f);

		FString HPText = FString::Printf(TEXT("Base HP: %d/%d"), Fighter->GetBaseHP(), Fighter->GetBaseMaxHP());
		DrawCenteredText(HPText, CY + 60.0f, PromptColor, 1.0f);

		DrawCenteredText(WaveNextMessage, CY + 120.0f, PromptColor, 1.3f);
	}
}
