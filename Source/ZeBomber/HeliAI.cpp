// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeliAI.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

AHeliAI::AHeliAI()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create scene root component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create static mesh component
	HeliMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeliMesh"));
	HeliMesh->SetupAttachment(RootComponent);
}

void AHeliAI::BeginPlay()
{
	Super::BeginPlay();

	// Store initial spawn location
	InitialSpawnLocation = GetActorLocation();

	UE_LOG(LogTemp, Error, TEXT("HeliAI: BeginPlay - Location=%s"), *GetActorLocation().ToString());

	// Check and log mesh status
	if (HeliMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("HeliAI: HeliMesh exists"));
		UE_LOG(LogTemp, Error, TEXT("HeliAI: Mesh asset=%s"), HeliMesh->GetStaticMesh() ? *HeliMesh->GetStaticMesh()->GetName() : TEXT("NULL"));
		UE_LOG(LogTemp, Error, TEXT("HeliAI: IsVisible=%d IsHiddenInGame=%d"), HeliMesh->IsVisible() ? 1 : 0, HeliMesh->bHiddenInGame ? 1 : 0);
		UE_LOG(LogTemp, Error, TEXT("HeliAI: RelativeLocation=%s Scale=%s"), *HeliMesh->GetRelativeLocation().ToString(), *HeliMesh->GetRelativeScale3D().ToString());
		
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

void AHeliAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bTargetSet)
	{
		return;
	}

	// Move and rotate toward target
	MoveTowardTarget(DeltaTime);
	RotateTowardTarget(DeltaTime);
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
