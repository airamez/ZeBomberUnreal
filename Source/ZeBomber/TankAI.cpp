// Copyright Epic Games, Inc. All Rights Reserved.

#include "TankAI.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"

ATankAI::ATankAI()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Create scene component as root (for actor rotation/movement)
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create the skeletal mesh component as child (for visual rotation)
	TankMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TankMesh"));
	TankMesh->SetupAttachment(SceneRoot);

	// Set default collision
	TankMesh->SetCollisionProfileName(TEXT("Pawn"));
	TankMesh->SetSimulatePhysics(false);

	// Default target is world origin
	TargetLocation = FVector::ZeroVector;
}

void ATankAI::BeginPlay()
{
	Super::BeginPlay();
}

void ATankAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	MoveTowardTarget(DeltaTime);
}

void ATankAI::SetTargetLocation(const FVector& NewTarget)
{
	TargetLocation = NewTarget;
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

	RotateTowardTarget(DeltaTime);

	// Move forward in the direction the tank is facing
	FVector CurrentLocation = GetActorLocation();
	FVector ForwardDirection = GetActorForwardVector();
	
	// Flatten to XY plane for ground movement
	ForwardDirection.Z = 0.0f;
	ForwardDirection.Normalize();

	FVector NewLocation = CurrentLocation + (ForwardDirection * MoveSpeed * DeltaTime);
	
	// Keep Z position same (ground level)
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
