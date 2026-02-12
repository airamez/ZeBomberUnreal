// Copyright Epic Games, Inc. All Rights Reserved.

#include "TankAI.h"
#include "FighterPawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

ATankAI::ATankAI()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	// Create explosion component for death effects
	ExplosionComp = CreateDefaultSubobject<UExplosionComponent>(TEXT("ExplosionComp"));

	// Create scene component as root (for actor rotation/movement)
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create box collision component for reliable hit detection
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(SceneRoot);
	CollisionBox->SetBoxExtent(FVector(150.0f, 75.0f, 75.0f));
	CollisionBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionBox->SetGenerateOverlapEvents(true);
	CollisionBox->SetSimulatePhysics(false);
	CollisionBox->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

	// Create the skeletal mesh component as child (for visual rotation)
	TankMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TankMesh"));
	TankMesh->SetupAttachment(SceneRoot);

	// Disable collision on mesh - CollisionBox handles it
	TankMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TankMesh->SetSimulatePhysics(false);

	// Default target is world origin
	TargetLocation = FVector::ZeroVector;
}

void ATankAI::BeginPlay()
{
	Super::BeginPlay();
}

void ATankAI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Spawn explosion effect when tank is destroyed
	if (ExplosionComp && EndPlayReason == EEndPlayReason::Destroyed)
	{
		UE_LOG(LogTemp, Log, TEXT("TankAI: Spawning explosion at owner location"));
		ExplosionComp->SpawnExplosionAtOwner();
	}

	Super::EndPlay(EndPlayReason);
}

void ATankAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Don't update AI when game is paused
	if (IsGamePaused())
	{
		return;
	}

	// Initialize zigzag on first tick if enabled but not yet initialized
	if (bUseZigzagMovement && !bZigzagInitialized)
	{
		InitializeZigzagMovement();
	}

	MoveTowardTarget(DeltaTime);

	// Fire at base when stopped at line of fire
	if (HasReachedTarget() && !bIsFiring)
	{
		bIsFiring = true;
		FireTimer = RateOfFire;
	}

	if (bIsFiring)
	{
		FireTimer -= DeltaTime;
		if (FireTimer <= 0.0f)
		{
			FireAtBase();
			FireTimer = RateOfFire;
		}
	}
}

void ATankAI::SetTargetLocation(const FVector& NewTarget)
{
	TargetLocation = NewTarget;
	bTargetSet = true;
	
	UE_LOG(LogTemp, Warning, TEXT("SetTargetLocation called: bUseZigzagMovement=%d"), bUseZigzagMovement ? 1 : 0);
	
	// If zigzag is enabled, reinitialize with new target
	if (bUseZigzagMovement)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetTargetLocation: Calling InitializeZigzagMovement"));
		InitializeZigzagMovement();
	}
	
	UE_LOG(LogTemp, Log, TEXT("TankAI: Target set to %s"), *TargetLocation.ToString());
}

void ATankAI::SetMoveSpeed(float NewSpeed)
{
	MoveSpeed = FMath::Max(0.0f, NewSpeed);
}

void ATankAI::SetStoppingDistance(float NewDistance)
{
	StoppingDistance = FMath::Max(0.0f, NewDistance);
}

void ATankAI::SetMeshRotation(float YawRotation)
{
	MeshRotationOffset = YawRotation;
	
	// Apply the rotation offset to the mesh (visual only)
	if (TankMesh)
	{
		FRotator RelativeRotation = TankMesh->GetRelativeRotation();
		RelativeRotation.Yaw = MeshRotationOffset;
		TankMesh->SetRelativeRotation(RelativeRotation);
	}
}

void ATankAI::SetZigzagSettings(bool bEnableZigzag, float MinDistance, float MaxDistance)
{
	bUseZigzagMovement = bEnableZigzag;
	ZigzagMinDistance = FMath::Max(0.0f, MinDistance);
	ZigzagMaxDistance = FMath::Max(0.0f, MaxDistance);
	
	UE_LOG(LogTemp, Warning, TEXT("SetZigzagSettings called: bEnableZigzag=%d, bTargetSet=%d"), bEnableZigzag ? 1 : 0, bTargetSet ? 1 : 0);
	
	// Reinitialize zigzag if enabled and target has been set
	if (bUseZigzagMovement && bTargetSet)
	{
		InitializeZigzagMovement();
	}
}

void ATankAI::SetRateOfFire(float Rate)
{
	RateOfFire = FMath::Max(0.1f, Rate);
}

bool ATankAI::HasReachedTarget() const
{
	float DistanceToTarget = FVector::Dist2D(GetActorLocation(), TargetLocation);
	return DistanceToTarget <= StoppingDistance;
}

void ATankAI::MoveTowardTarget(float DeltaTime)
{
	if (HasReachedTarget())
	{
		return;
	}

	if (bUseZigzagMovement)
	{
		MoveZigzag(DeltaTime);
	}
	else
	{
		// Standard direct movement
		RotateTowardTarget(DeltaTime);

		FVector CurrentLocation = GetActorLocation();
		FVector ForwardDirection = GetActorForwardVector();
		
		ForwardDirection.Z = 0.0f;
		ForwardDirection.Normalize();

		FVector NewLocation = CurrentLocation + (ForwardDirection * MoveSpeed * DeltaTime);
		NewLocation.Z = CurrentLocation.Z;

		SetActorLocation(NewLocation);
	}
}

void ATankAI::MoveZigzag(float DeltaTime)
{
	// Periodic debug logging (throttled per-instance via frame count)
	static int32 LogCounter = 0;
	if (++LogCounter % 300 == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("MoveZigzag: bHasCrossedCenter=%d, Location=%s, Target=%s"),
			bHasCrossedCenter ? 1 : 0, *GetActorLocation().ToString(), *TargetLocation.ToString());
	}
	
	// Check if we need to turn (crossed center and traveled far enough)
	if (!bHasCrossedCenter)
	{
		if (HasCrossedCenterLine())
		{
			bHasCrossedCenter = true;
			// Set remaining distance to travel after crossing
			RemainingZigzagDistance = FMath::FRandRange(ZigzagMinDistance, ZigzagMaxDistance);
			UE_LOG(LogTemp, Warning, TEXT("MoveZigzag: Crossed center! RemainingDistance=%.1f"), RemainingZigzagDistance);
		}
	}
	else
	{
		// We've crossed center, now travel the remaining distance before turning
		float DistanceThisFrame = MoveSpeed * DeltaTime;
		RemainingZigzagDistance -= DistanceThisFrame;
		
		if (RemainingZigzagDistance <= 0.0f)
		{
			// Time to turn!
			UE_LOG(LogTemp, Warning, TEXT("MoveZigzag: Turning! Distance reached 0"));
			UpdateZigzagDirection();
		}
	}

	// Rotate toward current zigzag angle
	RotateTowardZigzagAngle(DeltaTime);

	// Move forward in the direction we're facing
	FVector CurrentLocation = GetActorLocation();
	FVector ForwardDirection = GetActorForwardVector();
	
	ForwardDirection.Z = 0.0f;
	ForwardDirection.Normalize();

	FVector NewLocation = CurrentLocation + (ForwardDirection * MoveSpeed * DeltaTime);
	NewLocation.Z = CurrentLocation.Z;

	SetActorLocation(NewLocation);
}

void ATankAI::RotateTowardTarget(float DeltaTime)
{
	FVector CurrentLocation = GetActorLocation();
	FVector DirectionToTarget = TargetLocation - CurrentLocation;
	
	// Flatten to XY plane
	DirectionToTarget.Z = 0.0f;
	
	if (DirectionToTarget.IsNearlyZero())
	{
		return;
	}

	DirectionToTarget.Normalize();

	// Get target rotation (actor faces target, mesh has separate offset)
	FRotator TargetRotation = DirectionToTarget.Rotation();
	FRotator CurrentRotation = GetActorRotation();

	// Smoothly interpolate rotation
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed);
	
	// Keep only yaw rotation (tank stays flat on ground)
	NewRotation.Pitch = 0.0f;
	NewRotation.Roll = 0.0f;

	SetActorRotation(NewRotation);
}

void ATankAI::InitializeZigzagMovement()
{
	// Store initial spawn location
	InitialSpawnLocation = GetActorLocation();
	
	// Calculate the center angle (direct line from spawn to target)
	CenterAngleRad = CalculateCenterAngle();
	
	// Randomly choose initial direction (left or right)
	ZigzagDirection = FMath::RandBool() ? 1 : -1;
	
	// Set initial movement angle (center +/- 45 degrees)
	CurrentMovementAngleRad = CenterAngleRad + (ZigzagDirection * FMath::DegreesToRadians(45.0f));
	
	// Reset state
	bHasCrossedCenter = false;
	RemainingZigzagDistance = 0.0f;
	bZigzagInitialized = true;
	
	UE_LOG(LogTemp, Warning, TEXT("TankAI: Zigzag initialized - Spawn=%s, Target=%s, CenterAngle=%.1f, ZigzagAngle=%.1f, Direction=%d"),
		*InitialSpawnLocation.ToString(), *TargetLocation.ToString(),
		FMath::RadiansToDegrees(CenterAngleRad), FMath::RadiansToDegrees(CurrentMovementAngleRad), ZigzagDirection);
}

void ATankAI::UpdateZigzagDirection()
{
	// Flip direction
	ZigzagDirection *= -1;
	
	// Calculate new movement angle (center +/- 45 degrees)
	CurrentMovementAngleRad = CenterAngleRad + (ZigzagDirection * FMath::DegreesToRadians(45.0f));
	
	// Reset crossing state for next leg
	bHasCrossedCenter = false;
	RemainingZigzagDistance = 0.0f;
	
	UE_LOG(LogTemp, Log, TEXT("TankAI: Zigzag turn - New Direction=%d, NewAngle=%.1f"),
		ZigzagDirection, FMath::RadiansToDegrees(CurrentMovementAngleRad));
}

float ATankAI::CalculateCenterAngle() const
{
	FVector DirectionToTarget = TargetLocation - InitialSpawnLocation;
	DirectionToTarget.Z = 0.0f;
	
	if (DirectionToTarget.IsNearlyZero())
	{
		return 0.0f;
	}
	
	// Calculate angle using atan2
	return FMath::Atan2(DirectionToTarget.Y, DirectionToTarget.X);
}

bool ATankAI::HasCrossedCenterLine() const
{
	FVector CurrentLocation = GetActorLocation();
	
	// Vector from spawn to current position
	FVector SpawnToCurrent = CurrentLocation - InitialSpawnLocation;
	SpawnToCurrent.Z = 0.0f;
	
	// Vector from spawn to target (center line)
	FVector SpawnToTarget = TargetLocation - InitialSpawnLocation;
	SpawnToTarget.Z = 0.0f;
	
	if (SpawnToTarget.IsNearlyZero())
	{
		return false;
	}
	
	SpawnToTarget.Normalize();
	
	// Project current position onto center line
	float Projection = FVector::DotProduct(SpawnToCurrent, SpawnToTarget);
	
	// The center point is where the perpendicular from current position intersects the center line
	FVector CenterPoint = InitialSpawnLocation + (SpawnToTarget * Projection);
	
	// Check if we've passed the center point (distance from spawn is greater than center point distance)
	float DistCurrentFromSpawn = SpawnToCurrent.Size();
	float DistCenterFromSpawn = (CenterPoint - InitialSpawnLocation).Size();
	
	return DistCurrentFromSpawn > DistCenterFromSpawn + 50.0f; // Small threshold to ensure we've actually crossed
}

void ATankAI::FireAtBase()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (AFighterPawn* Fighter = Cast<AFighterPawn>(PlayerPawn))
	{
		Fighter->DamageBase(1);
	}
}

void ATankAI::RotateTowardZigzagAngle(float DeltaTime)
{
	// Convert current movement angle to rotation
	FRotator TargetRotation;
	TargetRotation.Yaw = FMath::RadiansToDegrees(CurrentMovementAngleRad);
	TargetRotation.Pitch = 0.0f;
	TargetRotation.Roll = 0.0f;
	
	FRotator CurrentRotation = GetActorRotation();
	
	// Smoothly interpolate rotation
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed);
	NewRotation.Pitch = 0.0f;
	NewRotation.Roll = 0.0f;
	
	SetActorRotation(NewRotation);
}

bool ATankAI::IsGamePaused() const
{
	// Get the player pawn and check if game is paused
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (AFighterPawn* Fighter = Cast<AFighterPawn>(PC->GetPawn()))
			{
				return Fighter->GetGameState() == EGameState::Paused;
			}
		}
	}
	return false;
}
