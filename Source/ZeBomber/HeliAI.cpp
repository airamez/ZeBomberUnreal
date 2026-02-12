// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeliAI.h"
#include "FighterPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"

AHeliAI::AHeliAI()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create explosion component for death effects
	ExplosionComp = CreateDefaultSubobject<UExplosionComponent>(TEXT("ExplosionComp"));

	// Create scene root component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Movable);
	RootComponent = SceneRoot;

	// Create static mesh component
	HeliMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeliMesh"));
	HeliMesh->SetupAttachment(RootComponent);
}

void AHeliAI::BeginPlay()
{
	Super::BeginPlay();

	// Ensure root component is Movable (Blueprint may have overridden the C++ default)
	if (SceneRoot && SceneRoot->Mobility != EComponentMobility::Movable)
	{
		SceneRoot->SetMobility(EComponentMobility::Movable);
	}

	// Store initial spawn location
	InitialSpawnLocation = GetActorLocation();

	UE_LOG(LogTemp, Log, TEXT("HeliAI: BeginPlay - Location=%s"), *GetActorLocation().ToString());

	// Check and log mesh status
	if (HeliMesh)
	{
		UE_LOG(LogTemp, Log, TEXT("HeliAI: Mesh asset=%s, Visible=%d"),
			HeliMesh->GetStaticMesh() ? *HeliMesh->GetStaticMesh()->GetName() : TEXT("NULL"),
			HeliMesh->IsVisible() ? 1 : 0);
		
		// Apply the rotation offset to the mesh (visual only)
		FRotator RelativeRotation = HeliMesh->GetRelativeRotation();
		RelativeRotation.Yaw = MeshRotationOffset;
		HeliMesh->SetRelativeRotation(RelativeRotation);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HeliAI: ERROR - HeliMesh is NULL!"));
	}
}

void AHeliAI::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Spawn explosion effect when helicopter is destroyed
	if (ExplosionComp && EndPlayReason == EEndPlayReason::Destroyed)
	{
		UE_LOG(LogTemp, Log, TEXT("HeliAI: Spawning explosion at owner location"));
		ExplosionComp->SpawnExplosionAtOwner();
	}

	Super::EndPlay(EndPlayReason);
}

void AHeliAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Don't update AI when game is paused
	if (IsGamePaused())
	{
		return;
	}

	if (!bTargetSet)
	{
		return;
	}

	// Move and rotate toward target
	MoveTowardTarget(DeltaTime);
	RotateTowardTarget(DeltaTime);

	// Fire at base when stopped at line of fire
	if (HasReachedTarget() && !bIsFiring)
	{
		bIsFiring = true;
		FireTimer = RateOfFire;
		UE_LOG(LogTemp, Log, TEXT("HeliAI: Reached target! Starting fire at base. Dist2D=%.1f, StopDist=%.1f"),
			FVector::Dist2D(GetActorLocation(), TargetLocation), StoppingDistance);
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

void AHeliAI::SetTargetLocation(const FVector& NewTarget)
{
	TargetLocation = NewTarget;
	bTargetSet = true;

	UE_LOG(LogTemp, Log, TEXT("HeliAI: Target set to %s"), *TargetLocation.ToString());
}

void AHeliAI::SetMoveSpeed(float NewSpeed)
{
	MoveSpeed = FMath::Max(0.0f, NewSpeed);
}

void AHeliAI::SetStoppingDistance(float NewDistance)
{
	StoppingDistance = FMath::Max(0.0f, NewDistance);
}

void AHeliAI::SetMeshRotation(float YawRotation)
{
	MeshRotationOffset = YawRotation;

	// Apply the rotation offset to the mesh (visual only)
	if (HeliMesh)
	{
		FRotator RelativeRotation = HeliMesh->GetRelativeRotation();
		RelativeRotation.Yaw = MeshRotationOffset;
		HeliMesh->SetRelativeRotation(RelativeRotation);
	}
}

void AHeliAI::SetFlyHeight(float NewHeight)
{
	FlyHeight = NewHeight;

	// Maintain the current XY position but set Z to the new fly height
	FVector CurrentLocation = GetActorLocation();
	CurrentLocation.Z = FlyHeight;
	SetActorLocation(CurrentLocation);
}

void AHeliAI::SetRateOfFire(float Rate)
{
	RateOfFire = FMath::Max(0.1f, Rate);
}

bool AHeliAI::HasReachedTarget() const
{
	if (!bTargetSet)
	{
		return false;
	}

	float Distance = FVector::Dist2D(GetActorLocation(), TargetLocation);
	return Distance <= StoppingDistance;
}

void AHeliAI::MoveTowardTarget(float DeltaTime)
{
	if (HasReachedTarget())
	{
		return;
	}

	// Calculate direction to target (only XY, maintain fly height)
	FVector CurrentLocation = GetActorLocation();
	FVector TargetAtHeight = TargetLocation;
	TargetAtHeight.Z = FlyHeight;

	FVector Direction = (TargetAtHeight - CurrentLocation).GetSafeNormal();
	Direction.Z = 0.0f; // Keep level flight

	// Move the helicopter
	FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;
	NewLocation.Z = FlyHeight; // Maintain fly height

	SetActorLocation(NewLocation);
}

void AHeliAI::FireAtBase()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (AFighterPawn* Fighter = Cast<AFighterPawn>(PlayerPawn))
	{
		Fighter->DamageBase(1);
	}
}

void AHeliAI::RotateTowardTarget(float DeltaTime)
{
	// Calculate rotation to face the target (only Yaw rotation)
	FVector CurrentLocation = GetActorLocation();
	FVector TargetAtHeight = TargetLocation;
	TargetAtHeight.Z = FlyHeight;

	FVector Direction = (TargetAtHeight - CurrentLocation).GetSafeNormal();
	Direction.Z = 0.0f;

	if (Direction.IsNearlyZero())
	{
		return;
	}

	FRotator TargetRotation = Direction.Rotation();
	FRotator CurrentRotation = GetActorRotation();

	// Smoothly interpolate toward target rotation (only Yaw)
	float NewYaw = FMath::FInterpTo(CurrentRotation.Yaw, TargetRotation.Yaw, DeltaTime, RotationSpeed);
	SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
}

bool AHeliAI::IsGamePaused() const
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
