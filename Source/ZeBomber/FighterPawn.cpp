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
#include "LandscapeProxy.h"
#include "WorldPartition/WorldPartition.h"
#include "Engine/World.h"

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


	// Create game state input actions
	PauseAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Pause_Auto"));
	PauseAction->ValueType = EInputActionValueType::Boolean;

	// Debug action for testing (Ctrl+Delete)
	DebugTestWaveAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_DebugTestWave_Auto"));
	DebugTestWaveAction->ValueType = EInputActionValueType::Boolean;

	ContinueAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Continue_Auto"));
	ContinueAction->ValueType = EInputActionValueType::Boolean;

	QuitAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Quit_Auto"));
	QuitAction->ValueType = EInputActionValueType::Boolean;

	// Create slide input actions (Q/E keys)
	SlideLeftAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_SlideLeft_Auto"));
	SlideLeftAction->ValueType = EInputActionValueType::Boolean;

	SlideRightAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_SlideRight_Auto"));
	SlideRightAction->ValueType = EInputActionValueType::Boolean;

	// Create radar zoom input actions ([ ] keys)
	RadarZoomInAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_RadarZoomIn_Auto"));
	RadarZoomInAction->ValueType = EInputActionValueType::Boolean;

	RadarZoomOutAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_RadarZoomOut_Auto"));
	RadarZoomOutAction->ValueType = EInputActionValueType::Boolean;

	// Create speed control input actions (mouse wheel)
	SpeedUpAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_SpeedUp_Auto"));
	SpeedUpAction->ValueType = EInputActionValueType::Boolean;

	SpeedDownAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_SpeedDown_Auto"));
	SpeedDownAction->ValueType = EInputActionValueType::Boolean;

	// Create FPS toggle input action (F key)
	FpsToggleAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_FpsToggle_Auto"));
	FpsToggleAction->ValueType = EInputActionValueType::Boolean;
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
	
	// Initialize slide velocity
	SlideVelocity = FVector::ZeroVector;

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

	// Configure landscape streaming for aerial view
	ConfigureLandscapeStreaming();

	// Create a mapping context for free-look RMB (always works, no Blueprint needed)
	UInputMappingContext* FreeLookMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_FreeLook_Auto"));
	FreeLookMappingContext->MapKey(FreeLookAction, EKeys::RightMouseButton);
	FreeLookMappingContext->MapKey(PauseAction, EKeys::Escape);
	FreeLookMappingContext->MapKey(ContinueAction, EKeys::C);
	FreeLookMappingContext->MapKey(QuitAction, EKeys::X);
	FreeLookMappingContext->MapKey(SlideLeftAction, EKeys::Q);
	FreeLookMappingContext->MapKey(SlideRightAction, EKeys::E);
	FreeLookMappingContext->MapKey(RadarZoomInAction, EKeys::LeftBracket);
	FreeLookMappingContext->MapKey(RadarZoomOutAction, EKeys::RightBracket);
	FreeLookMappingContext->MapKey(SpeedUpAction, EKeys::MouseScrollUp);
	FreeLookMappingContext->MapKey(SpeedDownAction, EKeys::MouseScrollDown);
	FreeLookMappingContext->MapKey(FpsToggleAction, EKeys::F);
	// Debug: Delete key = Test high-level wave
	FreeLookMappingContext->MapKey(DebugTestWaveAction, EKeys::Delete);
	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Created FreeLook mapping context with RMB, Q/E slide, [ ] zoom, mouse wheel speed, F key FPS toggle, Delete debug"));

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

		// Set raw mouse sensitivity to 1:1 for immediate response
		if (PC->PlayerInput)
		{
			PC->PlayerInput->SetMouseSensitivity(1.0f);
		}
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

	// Decay damage flash
	if (DamageFlashAlpha > 0.0f)
	{
		DamageFlashAlpha = FMath::Max(0.0f, DamageFlashAlpha - DamageFlashDecayRate * DeltaTime);
	}

	// Update FPS display timer (runs even when paused)
	if (bShowFps)
	{
		FpsUpdateTimer -= DeltaTime;
		if (FpsUpdateTimer <= 0.0f)
		{
			// Calculate current FPS and update display value
			CurrentFps = 1.0f / DeltaTime;
			FpsUpdateTimer = FpsUpdateInterval;
		}
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

		// Q = Slide Left
		if (SlideLeftAction)
		{
			EIC->BindAction(SlideLeftAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnSlideLeft);
		}

		// E = Slide Right
		if (SlideRightAction)
		{
			EIC->BindAction(SlideRightAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnSlideRight);
		}

		// [ = Radar Zoom In
		if (RadarZoomInAction)
		{
			EIC->BindAction(RadarZoomInAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnRadarZoomIn);
		}

		// ] = Radar Zoom Out
		if (RadarZoomOutAction)
		{
			EIC->BindAction(RadarZoomOutAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnRadarZoomOut);
		}

		// Mouse Wheel Up = Speed Up
		if (SpeedUpAction)
		{
			EIC->BindAction(SpeedUpAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnSpeedUp);
		}

		// Mouse Wheel Down = Speed Down
		if (SpeedDownAction)
		{
			EIC->BindAction(SpeedDownAction, ETriggerEvent::Triggered, this, &AFighterPawn::OnSpeedDown);
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

		// Debug: Ctrl+Delete = Test high-level wave
		if (DebugTestWaveAction)
		{
			EIC->BindAction(DebugTestWaveAction, ETriggerEvent::Started, this, &AFighterPawn::OnDebugTestWave);
		}

		// F key = Toggle FPS display
		if (FpsToggleAction)
		{
			EIC->BindAction(FpsToggleAction, ETriggerEvent::Started, this, &AFighterPawn::OnFpsToggle);
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

void AFighterPawn::OnSlideLeft(const FInputActionValue& Value)
{
	// Add left slide velocity (negative Y in local space)
	SlideVelocity.Y = -SlideSpeed;
}

void AFighterPawn::OnSlideRight(const FInputActionValue& Value)
{
	// Add right slide velocity (positive Y in local space)
	SlideVelocity.Y = SlideSpeed;
}

void AFighterPawn::OnRadarZoomIn(const FInputActionValue& Value)
{
	RadarZoom = FMath::Clamp(RadarZoom - RadarZoomStep, RadarZoomMin, RadarZoomMax);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Radar Zoom IN -> %.2f"), RadarZoom);
}

void AFighterPawn::OnRadarZoomOut(const FInputActionValue& Value)
{
	RadarZoom = FMath::Clamp(RadarZoom + RadarZoomStep, RadarZoomMin, RadarZoomMax);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Radar Zoom OUT -> %.2f"), RadarZoom);
}

void AFighterPawn::OnSpeedUp(const FInputActionValue& Value)
{
	DefaultSpeed = FMath::Clamp(DefaultSpeed + SpeedStep, MinSpeed, MaxSpeed);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Speed UP -> %.0f"), DefaultSpeed);
}

void AFighterPawn::OnSpeedDown(const FInputActionValue& Value)
{
	DefaultSpeed = FMath::Clamp(DefaultSpeed - SpeedStep, MinSpeed, MaxSpeed);
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Speed DOWN -> %.0f"), DefaultSpeed);
}

void AFighterPawn::OnDropBomb(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: OnDropBomb called - Warmup: %s, GameState: %d"), 
		bWarmupComplete ? TEXT("true") : TEXT("false"), (int32)CurrentGameState);
	
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


void AFighterPawn::OnPausePressed(const FInputActionValue& Value)
{
	if (CurrentGameState == EGameState::Playing)
	{
		CurrentGameState = EGameState::Paused;
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Game PAUSED"));
	}
}

void AFighterPawn::OnDebugTestWave(const FInputActionValue& Value)
{
	if (CurrentGameState != EGameState::Playing) return;

	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: DEBUG - Destroying all enemies except one of each type"));

	// Find all tanks and keep only one
	TArray<ATankAI*> AllTanks;
	for (TActorIterator<ATankAI> It(GetWorld()); It; ++It)
	{
		AllTanks.Add(*It);
	}

	// Destroy all tanks except the first one
	for (int32 i = 1; i < AllTanks.Num(); i++)
	{
		if (AllTanks[i] && AllTanks[i]->IsValidLowLevel())
		{
			AllTanks[i]->Destroy();
		}
	}

	// Find all helicopters and keep only one
	TArray<AHeliAI*> AllHelis;
	for (TActorIterator<AHeliAI> It(GetWorld()); It; ++It)
	{
		AllHelis.Add(*It);
	}

	// Destroy all helicopters except the first one
	for (int32 i = 1; i < AllHelis.Num(); i++)
	{
		if (AllHelis[i] && AllHelis[i]->IsValidLowLevel())
		{
			AllHelis[i]->Destroy();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: DEBUG - Kept 1 tank and 1 heli for testing. Destroyed %d tanks and %d helis."), 
		FMath::Max(0, AllTanks.Num() - 1), FMath::Max(0, AllHelis.Num() - 1));
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

void AFighterPawn::OnFpsToggle(const FInputActionValue& Value)
{
	bShowFps = !bShowFps;
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: FPS display %s"), bShowFps ? TEXT("ENABLED") : TEXT("DISABLED"));
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
	
	// Reset base HP at the start of each wave
	BaseHP = BaseMaxHP;
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Base HP reset to %d for wave %d"), BaseHP, CurrentWave);

	CurrentGameState = EGameState::Playing;
	
	// Reset mouse crosshair to center when game starts
	if (CurrentWave == 1)
	{
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC)
		{
			int32 SizeX, SizeY;
			PC->GetViewportSize(SizeX, SizeY);
			VirtualCursorPos = FVector2D(SizeX * 0.5f, SizeY * 0.5f);
			UE_LOG(LogTemp, Log, TEXT("FighterPawn: Mouse crosshair reset to center for game start"));
		}
	}

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
	FVector RightDirection = GetActorRightVector();
	
	// Decay slide velocity when not pressing keys
	if (FMath::Abs(SlideVelocity.Y) > 0.01f)
	{
		float DecayAmount = SlideDecayRate * DeltaTime;
		if (SlideVelocity.Y > 0.0f)
		{
			SlideVelocity.Y = FMath::Max(0.0f, SlideVelocity.Y - DecayAmount);
		}
		else
		{
			SlideVelocity.Y = FMath::Min(0.0f, SlideVelocity.Y + DecayAmount);
		}
	}
	else
	{
		SlideVelocity.Y = 0.0f;
	}
	
	// Calculate movement
	FVector Movement = (ForwardDirection * CurrentSpeed * DeltaTime) + (RightDirection * SlideVelocity.Y * DeltaTime);
	FVector NewLocation = GetActorLocation() + Movement;

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

	// Frame-time independent mouse movement for consistent feel at any framerate
	// Use 60fps as baseline (1/60 = 0.0167) to normalize movement
	float FrameScale = DeltaTime * 60.0f;
	
	// Apply mouse delta with sensitivity and frame scaling
	// Negate Y: UE positive DeltaY = mouse up, but screen Y increases down
	VirtualCursorPos.X += FrameMouseDeltaX * AimSensitivity * FrameScale;
	VirtualCursorPos.Y -= FrameMouseDeltaY * AimSensitivity * FrameScale;

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
		// Frame-time independent free-look movement
		float FrameScale = DeltaTime * 60.0f;
		
		// Use FreeLookSensitivity directly (degrees per pixel) with frame scaling
		FreeLookRotation.Yaw += FrameMouseDeltaX * FreeLookSensitivity * FrameScale;
		FreeLookRotation.Pitch += FrameMouseDeltaY * FreeLookSensitivity * FrameScale;

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
	 * The bomb inherits the airplane's velocity plus additional forward speed and then falls under gravity.
	 *   Total forward velocity = airplane forward * (CurrentSpeed + BombDropSpeed + BombHorizontalSpeed)
	 *   Vertical component  = velocity.Z * CurrentSpeed (initial)
	 *                         + 0.5 * g * t^2 (gravity pulls it down)
	 *
	 * We solve for t when Z reaches ground (Z = 0) using the quadratic formula,
	 * then compute the XY position at that time.
	 */

	FVector BombOrigin = GetActorLocation() + GetActorTransform().TransformVector(BombSpawnOffset);
	
	// Calculate total bomb velocity with additional forward speed
	FVector Velocity = GetActorForwardVector() * (CurrentSpeed + BombDropSpeed + BombHorizontalSpeed);

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
	UE_LOG(LogTemp, Warning, TEXT("FighterPawn: DropBomb called"));
	
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

			// Calculate bomb velocity with additional forward speed for distant targets
			FVector ForwardVelocity = GetActorForwardVector() * (CurrentSpeed + BombDropSpeed + BombHorizontalSpeed);
			FVector TotalBombVelocity = ForwardVelocity;
			
			BombPrimitive->SetPhysicsLinearVelocity(TotalBombVelocity);
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
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Helicopter destroyed! Wave: %d/%d"), WaveHelisDestroyed, WaveTotalHelis);
	}
}

// ==================== Landscape Streaming ====================

void AFighterPawn::ConfigureLandscapeStreaming()
{
	UE_LOG(LogTemp, Log, TEXT("FighterPawn: Configuring landscape streaming (distance=%.0f)"), LandscapeStreamingDistance);

	UWorld* World = GetWorld();
	if (!World) return;

	// Override the World Partition streaming source distances on the player controller
	// This tells the engine to load cells much further ahead of the player
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->bEnableStreamingSource = true;
		PC->StreamingSourceDebugColor = FColor::Green;
	}

	// Set landscape LOD bias via console variable — forces higher detail at distance
	// Negative values = prefer more detailed LODs, reducing mountain pop-in
	if (IConsoleVariable* LandscapeLODBias = IConsoleManager::Get().FindConsoleVariable(TEXT("r.LandscapeLODBias")))
	{
		LandscapeLODBias->Set(-2);
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Set r.LandscapeLODBias = -2"));
	}

	// Increase landscape LOD distribution scale for smoother transitions at distance
	if (IConsoleVariable* LODDistScale = IConsoleManager::Get().FindConsoleVariable(TEXT("r.LandscapeLODDistributionScale")))
	{
		LODDistScale->Set(3.0f);
		UE_LOG(LogTemp, Log, TEXT("FighterPawn: Set r.LandscapeLODDistributionScale = 3.0"));
	}

	// Force all existing landscape proxies to stay visible
	for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
	{
		ALandscapeProxy* Landscape = *It;
		if (Landscape)
		{
			Landscape->SetActorEnableCollision(true);
			Landscape->SetHidden(false);
			UE_LOG(LogTemp, Log, TEXT("FighterPawn: Configured landscape: %s"), *Landscape->GetName());
		}
	}
}

void AFighterPawn::UpdateLandscapeStreaming()
{
	// This can be called from Tick if needed for dynamic streaming updates
	// For now, the initial configuration should be sufficient for most cases
}
