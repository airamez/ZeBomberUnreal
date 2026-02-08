// Copyright Epic Games, Inc. All Rights Reserved.

#include "FighterPawn.h"
#include "RocketProjectile.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"

AFighterPawn::AFighterPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create scene root
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create first-person camera (at the nose of the invisible airplane)
	NoseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("NoseCamera"));
	NoseCamera->SetupAttachment(SceneRoot);
	NoseCamera->bUsePawnControlRotation = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
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

	// Apply camera offset and pitch
	if (NoseCamera)
	{
		NoseCamera->SetRelativeLocation(CameraOffset);
		NoseCamera->SetRelativeRotation(FRotator(CameraPitchOffset, 0.0f, 0.0f));
		UE_LOG(LogTemp, Warning, TEXT("FighterPawn: Camera configured - Offset=%s, PitchOffset=%.1f"),
			*CameraOffset.ToString(), CameraPitchOffset);
	}

	// Add input mapping context
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
		}
	}

	// Show mouse cursor for aiming
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = false;
		PC->bEnableMouseOverEvents = false;
		PC->CurrentMouseCursor = EMouseCursor::Crosshairs;
		PC->DefaultMouseCursor = EMouseCursor::Crosshairs;
	}

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

	UpdateFlight(DeltaTime);
	UpdateMouseAim();
	UpdateBombImpactPrediction();

	// Auto-fire rockets while button is held
	if (bFireRocketHeld)
	{
		FireRocket();
	}
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

// ==================== Flight Logic ====================

void AFighterPawn::UpdateFlight(float DeltaTime)
{
	FRotator CurrentRotation = GetActorRotation();

	// Apply inertia to inputs (smooth response)
	SmoothedPitchInput = FMath::Lerp(SmoothedPitchInput, PitchInput, 1.0f - PitchInertia);
	SmoothedYawInput = FMath::Lerp(SmoothedYawInput, YawInput, 1.0f - YawInertia);

	// --- Pitch (with inertia) ---
	if (FMath::Abs(SmoothedPitchInput) > 0.01f)
	{
		float PitchDelta = SmoothedPitchInput * PitchRate * DeltaTime;
		CurrentRotation.Pitch = FMath::Clamp(CurrentRotation.Pitch + PitchDelta, -MaxPitchAngle, MaxPitchAngle);
	}
	else
	{
		// Auto-level pitch when no input
		if (FMath::Abs(CurrentRotation.Pitch) > 0.5f)
		{
			CurrentRotation.Pitch = FMath::FInterpTo(CurrentRotation.Pitch, 0.0f, DeltaTime, LevelingSpeed);
		}
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

// ==================== Mouse Aim (White Crosshair) ====================

void AFighterPawn::UpdateMouseAim()
{
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (!PC) return;

	float MouseX, MouseY;
	if (PC->GetMousePosition(MouseX, MouseY))
	{
		FVector WorldLocation, WorldDirection;
		if (PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
		{
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

	// Don't fire backwards
	float DotForward = FVector::DotProduct(Direction, GetActorForwardVector());
	if (DotForward < 0.0f) return;

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
