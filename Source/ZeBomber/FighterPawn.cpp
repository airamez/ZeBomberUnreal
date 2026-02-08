// Copyright Epic Games, Inc. All Rights Reserved.

#include "FighterPawn.h"
#include "RocketProjectile.h"
#include "TankAI.h"
#include "HeliAI.h"
#include "TankWaveSpawner.h"
#include "HeliWaveSpawner.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerInput.h"
#include "Framework/Application/SlateApplication.h"
#include "EngineUtils.h"
#include "Kismet/KismetSystemLibrary.h"

AFighterPawn::AFighterPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create scene root
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create camera pivot for free-look (rotates independently of flight)
	CameraPivot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraPivot"));
	CameraPivot->SetupAttachment(SceneRoot);

	// Create first-person camera attached to pivot
	NoseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("NoseCamera"));
	NoseCamera->SetupAttachment(CameraPivot);
	NoseCamera->bUsePawnControlRotation = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// Create FreeLookAction in constructor so it's available for SetupPlayerInputComponent
	// (SetupPlayerInputComponent runs BEFORE BeginPlay)
	FreeLookAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_FreeLook_Auto"));
	FreeLookAction->ValueType = EInputActionValueType::Boolean;

	// Create ToggleJetHUDAction in constructor (/ key)
	ToggleJetHUDAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_ToggleJetHUD_Auto"));
	ToggleJetHUDAction->ValueType = EInputActionValueType::Boolean;

	// Create game state input actions
	PauseAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Pause_Auto"));
	PauseAction->ValueType = EInputActionValueType::Boolean;

	ContinueAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Continue_Auto"));
	ContinueAction->ValueType = EInputActionValueType::Boolean;

	QuitAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Quit_Auto"));
	QuitAction->ValueType = EInputActionValueType::Boolean;
}

void AFighterPawn::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: BeginPlay started"));

	// Warmup timer to prevent input from firing on first frames
	bWarmupComplete = false;
	WarmupTimer = 1.0f;

	// Set starting altitude
	FVector StartLocation = GetActorLocation();
	StartLocation.Z = StartAltitude;
	SetActorLocation(StartLocation);

	// Initialize speed
	CurrentSpeed = DefaultSpeed;

	// Apply camera offset and pitch to the camera (relative to pivot)
	if (NoseCamera)
	{
		NoseCamera->SetRelativeLocation(CameraOffset);
		NoseCamera->SetRelativeRotation(FRotator(CameraPitchOffset, 0.0f, 0.0f));
		UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Camera configured - Offset=%s, PitchOffset=%.1f"),
			*CameraOffset.ToString(), CameraPitchOffset);
	}

	// Initialize virtual cursor to screen center
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		int32 SizeX, SizeY;
		PC->GetViewportSize(SizeX, SizeY);
		VirtualCursorPos = FVector2D(SizeX * 0.5f, SizeY * 0.5f);
	}

	// Create a mapping context for free-look RMB (always works, no Blueprint needed)
	UInputMappingContext* FreeLookMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_FreeLook_Auto"));
	FreeLookMappingContext->MapKey(FreeLookAction, EKeys::RightMouseButton);
	FreeLookMappingContext->MapKey(ToggleJetHUDAction, EKeys::Slash);
	FreeLookMappingContext->MapKey(PauseAction, EKeys::Escape);
	FreeLookMappingContext->MapKey(ContinueAction, EKeys::C);
	FreeLookMappingContext->MapKey(QuitAction, EKeys::X);
	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Created FreeLook mapping context with RMB"));

	// Add input mapping contexts
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (FighterMappingContext)
			{
				Subsystem->AddMappingContext(FighterMappingContext, 0);
				UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Mapping context added successfully"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("FighterPawn: FighterMappingContext is NULL!"));
			}

			// Add the free-look mapping context at higher priority
			Subsystem->AddMappingContext(FreeLookMappingContext, 1);
			UE_LOG(LogTemp, Warning, TEXT("FighterPawn: FreeLook mapping context added"));
		}
	}

	// Hide OS mouse cursor and use Game-only input mode for zero-lag mouse
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->bShowMouseCursor = false;
		PC->bEnableClickEvents = false;
		PC->bEnableMouseOverEvents = false;

		// Game-only mode: raw mouse input, no Slate cursor processing = zero lag
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	// Bind to existing enemy destruction events for score tracking
	BindEnemyDestroyedEvents();

	// Start in Instructions state
	CurrentGameState = EGameState::Instructions;

	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Initialized at altitude %.0f, speed %.0f"), StartAltitude, CurrentSpeed);
}

void AFighterPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Warmup period - prevent any actions on first frames
	if (!bWarmupComplete)
	{
		WarmupTimer -= DeltaTime;
		if (WarmupTimer <= 0.0f)
		{
			bWarmupComplete = true;
			UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Warmup complete, input enabled"));
		}
		return;
	}

	// Read raw mouse delta ONCE per frame (consumed on read, so only call once)
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC)
	{
		PC->GetInputMouseDelta(FrameMouseDeltaX, FrameMouseDeltaY);
	}
	else
	{
		FrameMouseDeltaX = 0.0f;
		FrameMouseDeltaY = 0.0f;
		bFreeLookActive = false;
	}

	// Radar zoom via mouse scroll wheel
	if (PC)
	{
		if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp))
		{
			RadarZoom = FMath::Clamp(RadarZoom - RadarZoomStep, RadarZoomMin, RadarZoomMax);
			UE_LOG(LogTemp, Log, TEXT("FighterPawn: Radar Zoom IN -> %.2f"), RadarZoom);
		}
		else if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown))
		{
			RadarZoom = FMath::Clamp(RadarZoom + RadarZoomStep, RadarZoomMin, RadarZoomMax);
			UE_LOG(LogTemp, Log, TEXT("FighterPawn: Radar Zoom OUT -> %.2f"), RadarZoom);
		}
	}

	// Decay damage flash
	if (DamageFlashAlpha > 0.0f)
	{
		DamageFlashAlpha = FMath::Max(0.0f, DamageFlashAlpha - DamageFlashDecayRate * DeltaTime);
	}

	// Only run gameplay when Playing
	if (CurrentGameState != EGameState::Playing) return;

	UpdateFlight(DeltaTime);
	UpdateVirtualCursor(DeltaTime);
	UpdateFreeLook(DeltaTime);
	UpdateMouseAim();
	UpdateBombImpactPrediction();

	// Periodically re-bind to newly spawned enemies (every ~1 second)
	EnemyScanTimer -= DeltaTime;
	if (EnemyScanTimer <= 0.0f)
	{
		BindEnemyDestroyedEvents();
		EnemyScanTimer = 1.0f;
	}

	// Auto-fire rockets while button is held
	if (bFireRocketHeld)
	{
		FireRocket();
	}

	// Check if all enemies in wave are cleared
	// (also checked in AddTankKill/AddHeliKill but this catches edge cases)
}

void AFighterPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// W = Nose Down (tip down)
		if (PitchDownAction)
		{
			EIC->BindAction(PitchDownAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnPitchDown);
			EIC->BindAction(PitchDownAction, ETriggerEvent::Completed, this, &AFighterPawn::OnPitchDownReleased);
		}

		// S = Nose Up (tip up)
		if (PitchUpAction)
		{
			EIC->BindAction(PitchUpAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnPitchUp);
			EIC->BindAction(PitchUpAction, ETriggerEvent::Completed, this, &AFighterPawn::OnPitchUpReleased);
		}

		// A = Turn Left
		if (TurnLeftAction)
		{
			EIC->BindAction(TurnLeftAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnTurnLeft);
			EIC->BindAction(TurnLeftAction, ETriggerEvent::Completed, this, &AFighterPawn::OnTurnLeftReleased);
		}

		// D = Turn Right
		if (TurnRightAction)
		{
			EIC->BindAction(TurnRightAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnTurnRight);
			EIC->BindAction(TurnRightAction, ETriggerEvent::Completed, this, &AFighterPawn::OnTurnRightReleased);
		}

		// Space = Drop Bomb
		if (DropBombAction)
		{
			EIC->BindAction(DropBombAction, ETriggerEvent::Started, this, &AFighterPawn::OnDropBomb);
		}

		// Left Mouse = Fire Rocket
		if (FireRocketAction)
		{
			EIC->BindAction(FireRocketAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnFireRocket);
			EIC->BindAction(FireRocketAction, ETriggerEvent::Completed, this, &AFighterPawn::OnFireRocketReleased);
		}

		// Right Mouse = Free Look
		if (FreeLookAction)
		{
			EIC->BindAction(FreeLookAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnFreeLookPressed);
			EIC->BindAction(FreeLookAction, ETriggerEvent::Completed, this, &AFighterPawn::OnFreeLookReleased);
		}

		// / = Toggle Jet HUD
		if (ToggleJetHUDAction)
		{
			EIC->BindAction(ToggleJetHUDAction, ETriggerEvent::Started, this, &AFighterPawn::OnToggleJetHUD);
		}

		// ESC = Pause
		if (PauseAction)
		{
			EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &AFighterPawn::OnPausePressed);
		}

		// C = Continue / Start / Next Wave / Restart
		if (ContinueAction)
		{
			EIC->BindAction(ContinueAction, ETriggerEvent::Started, this, &AFighterPawn::OnContinuePressed);
		}

		// X = Quit
		if (QuitAction)
		{
			EIC->BindAction(QuitAction, ETriggerEvent::Started, this, &AFighterPawn::OnQuitGame);
		}

		// Volume controls (+ / -)
		if (VolumeUpAction)
		{
			EIC->BindAction(VolumeUpAction, ETriggerEvent::Started, this, &AFighterPawn::OnVolumeUp);
		}
		if (VolumeDownAction)
		{
			EIC->BindAction(VolumeDownAction, ETriggerEvent::Started, this, &AFighterPawn::OnVolumeDown);
		}

		// Sensitivity controls (< / >)
		if (SensitivityUpAction)
		{
			EIC->BindAction(SensitivityUpAction, ETriggerEvent::Started, this, &AFighterPawn::OnSensitivityUp);
		}
		if (SensitivityDownAction)
		{
			EIC->BindAction(SensitivityDownAction, ETriggerEvent::Started, this, &AFighterPawn::OnSensitivityDown);
		}
	}
}

// ==================== Input Handlers ====================

void AFighterPawn::OnPitchDown(const FInputActionValue& Value)
{
	PitchInput = -1.0f; // Nose down
}

void AFighterPawn::OnPitchDownReleased(const FInputActionValue& Value)
{
	PitchInput = 0.0f;
}

void AFighterPawn::OnPitchUp(const FInputActionValue& Value)
{
	PitchInput = 1.0f; // Nose up
}

void AFighterPawn::OnPitchUpReleased(const FInputActionValue& Value)
{
	PitchInput = 0.0f;
}

void AFighterPawn::OnTurnLeft(const FInputActionValue& Value)
{
	YawInput = -1.0f;
}

void AFighterPawn::OnTurnLeftReleased(const FInputActionValue& Value)
{
	YawInput = 0.0f;
}

void AFighterPawn::OnTurnRight(const FInputActionValue& Value)
{
	YawInput = 1.0f;
}

void AFighterPawn::OnTurnRightReleased(const FInputActionValue& Value)
{
	YawInput = 0.0f;
}

void AFighterPawn::OnDropBomb(const FInputActionValue& Value)
{
	if (!bWarmupComplete) return;
	if (CurrentGameState != EGameState::Playing) return;
	DropBomb();
}

void AFighterPawn::OnFireRocket(const FInputActionValue& Value)
{
	if (!bWarmupComplete) return;
	bFireRocketHeld = true;
}

void AFighterPawn::OnFireRocketReleased(const FInputActionValue& Value)
{
	bFireRocketHeld = false;
}

void AFighterPawn::OnFreeLookPressed(const FInputActionValue& Value)
{
	bFreeLookActive = true;
}

void AFighterPawn::OnFreeLookReleased(const FInputActionValue& Value)
{
	bFreeLookActive = false;
}

void AFighterPawn::OnVolumeUp(const FInputActionValue& Value)
{
	SoundVolume = FMath::Clamp(SoundVolume + VolumeStep, 0.0f, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Volume UP -> %.0f%%"), SoundVolume * 100.0f);
}

void AFighterPawn::OnVolumeDown(const FInputActionValue& Value)
{
	SoundVolume = FMath::Clamp(SoundVolume - VolumeStep, 0.0f, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Volume DOWN -> %.0f%%"), SoundVolume * 100.0f);
}

void AFighterPawn::OnSensitivityUp(const FInputActionValue& Value)
{
	AimSensitivity = FMath::Clamp(AimSensitivity + SensitivityStep, MinSensitivity, MaxSensitivity);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Sensitivity UP -> %.1f"), AimSensitivity);
}

void AFighterPawn::OnSensitivityDown(const FInputActionValue& Value)
{
	AimSensitivity = FMath::Clamp(AimSensitivity - SensitivityStep, MinSensitivity, MaxSensitivity);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Sensitivity DOWN -> %.1f"), AimSensitivity);
}

void AFighterPawn::OnToggleJetHUD(const FInputActionValue& Value)
{
	bJetHUDEnabled = !bJetHUDEnabled;
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Jet HUD %s"), bJetHUDEnabled ? TEXT("ON") : TEXT("OFF"));
}

void AFighterPawn::OnPausePressed(const FInputActionValue& Value)
{
	if (CurrentGameState == EGameState::Playing)
	{
		CurrentGameState = EGameState::Paused;
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Game PAUSED"));
	}
}

void AFighterPawn::OnContinuePressed(const FInputActionValue& Value)
{
	if (CurrentGameState == EGameState::Instructions || CurrentGameState == EGameState::WaveEnd)
	{
		StartNextWave();
	}
	else if (CurrentGameState == EGameState::Paused)
	{
		CurrentGameState = EGameState::Playing;
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Game RESUMED"));
	}
	else if (CurrentGameState == EGameState::GameOver)
	{
		UGameplayStatics::OpenLevel(GetWorld(), FName(*GetWorld()->GetName()));
	}
}

void AFighterPawn::OnQuitGame(const FInputActionValue& Value)
{
	if (CurrentGameState == EGameState::Paused)
	{
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Quitting game"));
		UKismetSystemLibrary::QuitGame(GetWorld(), Cast<APlayerController>(Controller), EQuitPreference::Quit, false);
	}
}

void AFighterPawn::DamageBase(int32 Damage)
{
	if (CurrentGameState != EGameState::Playing) return;

	BaseHP = FMath::Max(0, BaseHP - Damage);
	DamageFlashAlpha = 0.6f;
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Base hit! HP: %d/%d"), BaseHP, BaseMaxHP);

	if (BaseHP <= 0)
	{
		CurrentGameState = EGameState::GameOver;
		UE_LOG(LogTemp, Warning, TEXT("FighterPawn: GAME OVER - Base destroyed!"));
	}
}

void AFighterPawn::RegisterWaveEnemies(int32 Tanks, int32 Helis)
{
	WaveTotalTanks += Tanks;
	WaveTotalHelis += Helis;
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Wave enemies registered - Tanks: %d, Helis: %d"), WaveTotalTanks, WaveTotalHelis);
}

void AFighterPawn::CheckWaveCleared()
{
	if (CurrentGameState != EGameState::Playing) return;

	int32 TotalKilled = WaveTanksDestroyed + WaveHelisDestroyed;
	int32 TotalEnemies = WaveTotalTanks + WaveTotalHelis;

	if (TotalEnemies > 0 && TotalKilled >= TotalEnemies)
	{
		WaveDuration = GetWorld()->GetTimeSeconds() - WaveStartTime;
		CurrentGameState = EGameState::WaveEnd;
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Wave %d cleared in %.1f seconds!"), CurrentWave, WaveDuration);
	}
}

void AFighterPawn::StartNextWave()
{
	CurrentWave++;
	WaveTanksDestroyed = 0;
	WaveHelisDestroyed = 0;
	WaveTotalTanks = 0;
	WaveTotalHelis = 0;
	WaveStartTime = GetWorld()->GetTimeSeconds();

	CurrentGameState = EGameState::Playing;

	// Find spawners and trigger them
	for (TActorIterator<ATankWaveSpawner> It(GetWorld()); It; ++It)
	{
		int32 TankCount = It->GetNextWaveTankCount();
		It->TriggerNextWave();
		RegisterWaveEnemies(TankCount, 0);
	}

	for (TActorIterator<AHeliWaveSpawner> It(GetWorld()); It; ++It)
	{
		int32 HeliCount = It->GetNextWaveHeliCount();
		It->TriggerNextWave();
		RegisterWaveEnemies(0, HeliCount);
	}

	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Wave %d started! Tanks: %d, Helis: %d"), CurrentWave, WaveTotalTanks, WaveTotalHelis);
}

// ==================== Flight Logic ====================

void AFighterPawn::UpdateFlight(float DeltaTime)
{
	FRotator CurrentRotation = GetActorRotation();

	// Apply inertia to inputs (smooth response)
	SmoothedPitchInput = FMath::Lerp(SmoothedPitchInput, PitchInput, 1.0f - PitchInertia);
	SmoothedYawInput = FMath::Lerp(SmoothedYawInput, YawInput, 1.0f - YawInertia);

	// --- Pitch (with inertia) ---
	// Airplane holds its current pitch when no input is pressed.
	// Only changes when the player actively presses W (tip down) or S (tip up).
	if (FMath::Abs(SmoothedPitchInput) > 0.01f)
	{
		float PitchDelta = SmoothedPitchInput * PitchRate * DeltaTime;
		CurrentRotation.Pitch = FMath::Clamp(CurrentRotation.Pitch + PitchDelta, -MaxPitchAngle, MaxPitchAngle);
	}

	// --- Yaw (turning with inertia) ---
	if (FMath::Abs(SmoothedYawInput) > 0.01f)
	{
		float YawDelta = SmoothedYawInput * YawRate * DeltaTime;
		CurrentRotation.Yaw += YawDelta;
	}

	// --- Roll (visual banking when turning) ---
	float TargetRoll = YawInput * MaxRollAngle;
	CurrentRotation.Roll = FMath::FInterpTo(CurrentRotation.Roll, TargetRoll, DeltaTime, RollRate / MaxRollAngle * 5.0f);

	// Apply rotation
	SetActorRotation(CurrentRotation);

	// --- Speed adjustment based on pitch ---
	float PitchFactor = -CurrentRotation.Pitch / MaxPitchAngle;
	float TargetSpeed = DefaultSpeed + (PitchFactor * SpeedChangeRate);
	TargetSpeed = FMath::Clamp(TargetSpeed, MinSpeed, MaxSpeed);
	CurrentSpeed = FMath::FInterpTo(CurrentSpeed, TargetSpeed, DeltaTime, 2.0f);

	// --- Movement ---
	FVector ForwardDirection = GetActorForwardVector();
	FVector NewLocation = GetActorLocation() + (ForwardDirection * CurrentSpeed * DeltaTime);

	// Enforce minimum altitude
	if (NewLocation.Z < MinAltitude)
	{
		NewLocation.Z = MinAltitude;
		if (CurrentRotation.Pitch < -5.0f)
		{
			CurrentRotation.Pitch = FMath::FInterpTo(CurrentRotation.Pitch, 0.0f, DeltaTime, LevelingSpeed * 2.0f);
			SetActorRotation(CurrentRotation);
		}
	}

	SetActorLocation(NewLocation);
}

// ==================== Virtual Cursor (Zero-Lag) ====================

void AFighterPawn::UpdateVirtualCursor(float DeltaTime)
{
	// During free-look, mouse moves the camera, not the crosshair
	if (bFreeLookActive) return;

	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return;

	// Use cached frame mouse delta (negate Y: UE positive DeltaY = mouse up, but screen Y increases down)
	VirtualCursorPos.X += FrameMouseDeltaX * AimSensitivity;
	VirtualCursorPos.Y -= FrameMouseDeltaY * AimSensitivity;

	// Clamp to viewport bounds
	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	VirtualCursorPos.X = FMath::Clamp(VirtualCursorPos.X, 0.0f, static_cast<float>(SizeX));
	VirtualCursorPos.Y = FMath::Clamp(VirtualCursorPos.Y, 0.0f, static_cast<float>(SizeY));
}

// ==================== Free-Look Camera ====================

void AFighterPawn::UpdateFreeLook(float DeltaTime)
{
	if (!CameraPivot) return;

	if (bFreeLookActive)
	{
		// Use cached frame mouse delta for free-look rotation with separate sensitivity
		float EffectiveFreeLookSensitivity = AimSensitivity * FreeLookSensitivityMultiplier;
		FreeLookRotation.Yaw += FrameMouseDeltaX * EffectiveFreeLookSensitivity;
		FreeLookRotation.Pitch += FrameMouseDeltaY * EffectiveFreeLookSensitivity;

		// Clamp free-look angles
		FreeLookRotation.Yaw = FMath::Clamp(FreeLookRotation.Yaw, -FreeLookMaxYaw, FreeLookMaxYaw);
		FreeLookRotation.Pitch = FMath::Clamp(FreeLookRotation.Pitch, -FreeLookMaxPitch, FreeLookMaxPitch);

		CameraPivot->SetRelativeRotation(FreeLookRotation);
	}
	else
	{
		// Smoothly return camera to forward position
		if (!FreeLookRotation.IsNearlyZero(0.1f))
		{
			FreeLookRotation = FMath::RInterpTo(FreeLookRotation, FRotator::ZeroRotator, DeltaTime, FreeLookReturnSpeed);
			CameraPivot->SetRelativeRotation(FreeLookRotation);
		}
		else if (!FreeLookRotation.IsZero())
		{
			FreeLookRotation = FRotator::ZeroRotator;
			CameraPivot->SetRelativeRotation(FRotator::ZeroRotator);
		}
	}
}

// ==================== Mouse Aim (White Crosshair) ====================

void AFighterPawn::UpdateMouseAim()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return;

	// Always deproject VirtualCursorPos through the current camera.
	// During free-look the cursor is frozen on screen but the camera rotates,
	// so the same screen position aims in a new world direction — this lets
	// the player aim the rocket turret by looking around.
	FVector WorldLocation, WorldDirection;
	PC->DeprojectScreenPositionToWorld(VirtualCursorPos.X, VirtualCursorPos.Y, WorldLocation, WorldDirection);

	FVector TraceStart = WorldLocation;
	FVector TraceEnd = WorldLocation + (WorldDirection * CrosshairMaxDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		RocketAimWorldTarget = HitResult.ImpactPoint;
	}
	else
	{
		RocketAimWorldTarget = TraceEnd;
	}
}

// ==================== Bomb Impact Prediction (Red Crosshair) ====================

void AFighterPawn::UpdateBombImpactPrediction()
{
	/*
	 * Predict where a bomb dropped NOW would land.
	 *
	 * The bomb inherits the airplane's velocity and then falls under gravity.
	 *   Horizontal velocity = airplane forward * CurrentSpeed  (constant, no drag)
	 *   Vertical component  = airplane forward.Z * CurrentSpeed (initial)
	 *                         + 0.5 * g * t^2 (gravity pulls it down)
	 *
	 * We solve for t when Z reaches ground (Z = 0) using the quadratic formula,
	 * then compute the XY position at that time.
	 */

	FVector BombOrigin = GetActorLocation() + GetActorTransform().TransformVector(BombSpawnOffset);
	FVector Velocity = GetActorForwardVector() * (CurrentSpeed + BombDropSpeed);

	float Vz = Velocity.Z;
	float H = BombOrigin.Z; // height above ground (ground = Z 0)

	// Quadratic: 0.5*g*t^2 - Vz*t - H = 0  (solving for when altitude = 0)
	// Using: z(t) = H + Vz*t - 0.5*g*t^2 = 0
	// => 0.5*g*t^2 - Vz*t - H = 0
	float a = 0.5f * BombGravity;
	float b = -Vz;
	float c = -H;

	float Discriminant = b * b - 4.0f * a * c;

	if (Discriminant < 0.0f)
	{
		// No solution (shouldn't happen if we're above ground)
		bBombImpactValid = false;
		return;
	}

	float SqrtDisc = FMath::Sqrt(Discriminant);
	float t1 = (-b + SqrtDisc) / (2.0f * a);
	float t2 = (-b - SqrtDisc) / (2.0f * a);

	// We want the positive root (future time)
	float FallTime = (t1 > 0.0f) ? t1 : t2;
	if (FallTime <= 0.0f)
	{
		bBombImpactValid = false;
		return;
	}

	// Predicted impact position (horizontal movement is constant velocity)
	FVector ImpactPos;
	ImpactPos.X = BombOrigin.X + Velocity.X * FallTime;
	ImpactPos.Y = BombOrigin.Y + Velocity.Y * FallTime;
	ImpactPos.Z = 0.0f; // ground level

	// Raycast down from predicted position to find actual terrain height
	FVector TraceStart = FVector(ImpactPos.X, ImpactPos.Y, BombOrigin.Z + 1000.0f);
	FVector TraceEnd = FVector(ImpactPos.X, ImpactPos.Y, -10000.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		BombImpactPoint = HitResult.ImpactPoint;
		bBombImpactValid = true;
	}
	else
	{
		// No terrain found, use flat ground estimate
		BombImpactPoint = ImpactPos;
		bBombImpactValid = true;
	}
}

// ==================== Weapons ====================

void AFighterPawn::DropBomb()
{
	if (!BombClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("FighterPawn: No BombClass assigned!"));
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastBombDropTime < BombCooldown) return;
	LastBombDropTime = CurrentTime;

	FVector SpawnLocation = GetActorLocation() + GetActorTransform().TransformVector(BombSpawnOffset);
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	AActor* Bomb = GetWorld()->SpawnActor<AActor>(BombClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Bomb)
	{
		UPrimitiveComponent* BombPrimitive = Cast<UPrimitiveComponent>(Bomb->GetRootComponent());
		if (!BombPrimitive)
		{
			BombPrimitive = Bomb->FindComponentByClass<UPrimitiveComponent>();
		}

		if (BombPrimitive)
		{
			BombPrimitive->SetSimulatePhysics(true);
			BombPrimitive->SetEnableGravity(true);

			FVector BomberVelocity = GetActorForwardVector() * (CurrentSpeed + BombDropSpeed);
			BombPrimitive->SetPhysicsLinearVelocity(BomberVelocity);
		}

		// Play bomb release sound
		if (BombDropSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, BombDropSound, SpawnLocation, SoundVolume);
		}

		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Bomb dropped at %s with speed %.0f"), *SpawnLocation.ToString(), CurrentSpeed);
	}
}

void AFighterPawn::FireRocket()
{
	if (!RocketClass) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastRocketFireTime < RocketCooldown) return;
	LastRocketFireTime = CurrentTime;

	FVector SpawnLocation = GetActorLocation() + GetActorTransform().TransformVector(RocketSpawnOffset);

	// Direction from spawn point to mouse-aim target
	FVector Direction = (RocketAimWorldTarget - SpawnLocation).GetSafeNormal();

	if (RocketAimWorldTarget.IsNearlyZero())
	{
		Direction = GetActorForwardVector();
	}

	FRotator SpawnRotation = Direction.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	AActor* Rocket = GetWorld()->SpawnActor<AActor>(RocketClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Rocket)
	{
		if (ARocketProjectile* RocketProj = Cast<ARocketProjectile>(Rocket))
		{
			RocketProj->SetFlightDirection(Direction);
		}

		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Rocket fired toward %s"), *RocketAimWorldTarget.ToString());
	}
}

// ==================== Score Tracking ====================

void AFighterPawn::BindEnemyDestroyedEvents()
{
	if (!GetWorld()) return;

	// Bind to all TankAI actors
	for (TActorIterator<ATankAI> It(GetWorld()); It; ++It)
	{
		AActor* Tank = *It;
		if (!BoundEnemies.Contains(Tank))
		{
			Tank->OnDestroyed.AddDynamic(this, &AFighterPawn::OnEnemyDestroyed);
			BoundEnemies.Add(Tank);
		}
	}

	// Bind to all HeliAI actors
	for (TActorIterator<AHeliAI> It(GetWorld()); It; ++It)
	{
		AActor* Heli = *It;
		if (!BoundEnemies.Contains(Heli))
		{
			Heli->OnDestroyed.AddDynamic(this, &AFighterPawn::OnEnemyDestroyed);
			BoundEnemies.Add(Heli);
		}
	}
}

void AFighterPawn::OnEnemyDestroyed(AActor* DestroyedActor)
{
	BoundEnemies.Remove(DestroyedActor);

	if (DestroyedActor->IsA<ATankAI>())
	{
		AddTankKill();
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Tank destroyed! Wave: %d/%d"), WaveTanksDestroyed, WaveTotalTanks);
	}
	else if (DestroyedActor->IsA<AHeliAI>())
	{
		AddHeliKill();
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Heli destroyed! Wave: %d/%d"), WaveHelisDestroyed, WaveTotalHelis);
	}
}
