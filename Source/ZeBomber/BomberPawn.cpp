// Copyright Epic Games, Inc. All Rights Reserved.

#include "BomberPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

ABomberPawn::ABomberPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create scene root
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create mesh pivot for rotation offset
	MeshPivot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshPivot"));
	MeshPivot->SetupAttachment(SceneRoot);

	// Create bomber mesh attached to pivot
	BomberMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BomberMesh"));
	BomberMesh->SetupAttachment(MeshPivot);
	BomberMesh->SetCollisionProfileName(TEXT("Pawn"));
	BomberMesh->SetSimulatePhysics(false);

	// Create spring arm for camera
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(SceneRoot);
	CameraArm->TargetArmLength = CameraDistance;
	CameraArm->bUsePawnControlRotation = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = CameraLagSpeed;
	CameraArm->bEnableCameraRotationLag = true;
	CameraArm->CameraRotationLagSpeed = CameraRotationLagSpeed;
	CameraArm->bInheritPitch = true;
	CameraArm->bInheritYaw = true;
	CameraArm->bInheritRoll = false;

	// Create camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraArm);

	// Default target location
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void ABomberPawn::BeginPlay()
{
	Super::BeginPlay();

	// Set starting altitude
	FVector StartLocation = GetActorLocation();
	StartLocation.Z = StartAltitude;
	SetActorLocation(StartLocation);

	// Initialize speed
	CurrentSpeed = DefaultSpeed;

	// Apply camera settings
	if (CameraArm)
	{
		CameraArm->TargetArmLength = CameraDistance;
		CameraArm->SetRelativeRotation(FRotator(CameraPitchAngle, 0.0f, 0.0f));
		CameraArm->CameraLagSpeed = CameraLagSpeed;
		CameraArm->CameraRotationLagSpeed = CameraRotationLagSpeed;
	}

	// Apply mesh rotation offset via pivot (not affected by Blueprint overrides)
	if (MeshPivot)
	{
		MeshPivot->SetRelativeRotation(MeshRotationOffset);
		UE_LOG(LogTemp, Warning, TEXT("BomberPawn: MeshRotationOffset applied: Pitch=%.1f Yaw=%.1f Roll=%.1f"),
			MeshRotationOffset.Pitch, MeshRotationOffset.Yaw, MeshRotationOffset.Roll);
	}

	// Add input mapping context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (BomberMappingContext)
			{
				Subsystem->AddMappingContext(BomberMappingContext, 0);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("BomberPawn: Initialized at altitude %.0f, speed %.0f"), StartAltitude, CurrentSpeed);
}

void ABomberPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateFlight(DeltaTime);
}

void ABomberPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// W = Nose Down (pitch down)
		if (PitchDownAction)
		{
			EnhancedInputComponent->BindAction(PitchDownAction, ETriggerEvent::Triggered, this, &ABomberPawn::OnPitchDown);
			EnhancedInputComponent->BindAction(PitchDownAction, ETriggerEvent::Completed, this, &ABomberPawn::OnPitchDownReleased);
		}

		// S = Nose Up (pitch up)
		if (PitchUpAction)
		{
			EnhancedInputComponent->BindAction(PitchUpAction, ETriggerEvent::Triggered, this, &ABomberPawn::OnPitchUp);
			EnhancedInputComponent->BindAction(PitchUpAction, ETriggerEvent::Completed, this, &ABomberPawn::OnPitchUpReleased);
		}

		// A = Turn Left
		if (TurnLeftAction)
		{
			EnhancedInputComponent->BindAction(TurnLeftAction, ETriggerEvent::Triggered, this, &ABomberPawn::OnTurnLeft);
			EnhancedInputComponent->BindAction(TurnLeftAction, ETriggerEvent::Completed, this, &ABomberPawn::OnTurnLeftReleased);
		}

		// D = Turn Right
		if (TurnRightAction)
		{
			EnhancedInputComponent->BindAction(TurnRightAction, ETriggerEvent::Triggered, this, &ABomberPawn::OnTurnRight);
			EnhancedInputComponent->BindAction(TurnRightAction, ETriggerEvent::Completed, this, &ABomberPawn::OnTurnRightReleased);
		}

		// Space = Drop Bomb
		if (DropBombAction)
		{
			EnhancedInputComponent->BindAction(DropBombAction, ETriggerEvent::Started, this, &ABomberPawn::OnDropBomb);
		}
	}
}

// ==================== Input Handlers ====================

void ABomberPawn::OnPitchDown(const FInputActionValue& Value)
{
	PitchInput = -1.0f; // Nose down
}

void ABomberPawn::OnPitchDownReleased(const FInputActionValue& Value)
{
	PitchInput = 0.0f;
}

void ABomberPawn::OnPitchUp(const FInputActionValue& Value)
{
	PitchInput = 1.0f; // Nose up
}

void ABomberPawn::OnPitchUpReleased(const FInputActionValue& Value)
{
	PitchInput = 0.0f;
}

void ABomberPawn::OnTurnLeft(const FInputActionValue& Value)
{
	YawInput = -1.0f;
}

void ABomberPawn::OnTurnLeftReleased(const FInputActionValue& Value)
{
	YawInput = 0.0f;
}

void ABomberPawn::OnTurnRight(const FInputActionValue& Value)
{
	YawInput = 1.0f;
}

void ABomberPawn::OnTurnRightReleased(const FInputActionValue& Value)
{
	YawInput = 0.0f;
}

void ABomberPawn::OnDropBomb(const FInputActionValue& Value)
{
	DropBomb();
}

// ==================== Flight Logic ====================

void ABomberPawn::UpdateFlight(float DeltaTime)
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
			float LevelDelta = LevelingSpeed * DeltaTime;
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
	float TargetRoll = YawInput * MaxRollAngle; // Bank into the turn
	CurrentRotation.Roll = FMath::FInterpTo(CurrentRotation.Roll, TargetRoll, DeltaTime, RollRate / MaxRollAngle * 5.0f);

	// Apply rotation
	SetActorRotation(CurrentRotation);

	// --- Speed adjustment based on pitch ---
	// Diving (nose down) increases speed, climbing (nose up) decreases speed
	float PitchFactor = -CurrentRotation.Pitch / MaxPitchAngle; // -1 (climbing) to +1 (diving)
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
		// If we hit min altitude while diving, level out
		if (CurrentRotation.Pitch < -5.0f)
		{
			CurrentRotation.Pitch = FMath::FInterpTo(CurrentRotation.Pitch, 0.0f, DeltaTime, LevelingSpeed * 2.0f);
			SetActorRotation(CurrentRotation);
		}
	}

	SetActorLocation(NewLocation);
}

void ABomberPawn::DropBomb()
{
	if (!BombClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("BomberPawn: No BombClass assigned!"));
		return;
	}

	// Check cooldown
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastBombDropTime < BombCooldown)
	{
		return;
	}
	LastBombDropTime = CurrentTime;

	// Calculate spawn location (below the bomber)
	FVector SpawnLocation = GetActorLocation() + GetActorTransform().TransformVector(BombSpawnOffset);
	FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	AActor* Bomb = GetWorld()->SpawnActor<AActor>(BombClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Bomb)
	{
		// Find the bomb's root primitive component to apply physics
		UPrimitiveComponent* BombPrimitive = Cast<UPrimitiveComponent>(Bomb->GetRootComponent());
		if (!BombPrimitive)
		{
			// Try to find any primitive component
			BombPrimitive = Bomb->FindComponentByClass<UPrimitiveComponent>();
		}

		if (BombPrimitive)
		{
			// Enable physics
			BombPrimitive->SetSimulatePhysics(true);
			BombPrimitive->SetEnableGravity(true);

			// Apply the bomber's velocity to the bomb (inherit momentum)
			FVector BomberVelocity = GetActorForwardVector() * (CurrentSpeed + BombDropSpeed);
			BombPrimitive->SetPhysicsLinearVelocity(BomberVelocity);
		}

		UE_LOG(LogTemp, Log, TEXT("BomberPawn: Bomb dropped at %s with speed %.0f"), *SpawnLocation.ToString(), CurrentSpeed);
	}
}
