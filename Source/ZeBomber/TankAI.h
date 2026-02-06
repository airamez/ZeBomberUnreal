// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TankAI.generated.h"

UCLASS()
class ZEBOMBER_API ATankAI : public APawn
{
	GENERATED_BODY()

public:
	ATankAI();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** The skeletal mesh component for the tank */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	class USkeletalMeshComponent* TankMesh;

	/** The root scene component (for actor rotation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	class USceneComponent* SceneRoot;

	/** The target location to move toward (base/church position) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tank Movement")
	FVector TargetLocation;

	/** Movement speed in units per second */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tank Movement")
	float MoveSpeed = 200.0f;

	/** Distance to stop from target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Movement", meta = (ClampMin = "0.0"))
	float StoppingDistance = 100.0f;

	/** Rotation offset to fix tank model orientation (in degrees) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tank Movement")
	float MeshRotationOffset = 0.0f;

	/** Rotation speed when turning toward target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Movement", meta = (ClampMin = "0.0"))
	float RotationSpeed = 3.0f;

	/** Enable zigzag movement pattern (sailboat style) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tank Movement")
	bool bUseZigzagMovement = false;

	/** Minimum distance to travel after crossing center line before turning (zigzag) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tank Movement")
	float ZigzagMinDistance = 200.0f;

	/** Maximum distance to travel after crossing center line before turning (zigzag) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tank Movement")
	float ZigzagMaxDistance = 500.0f;

public:
	/** Set the target location for the tank to move toward */
	UFUNCTION(BlueprintCallable, Category = "Tank Movement")
	void SetTargetLocation(const FVector& NewTarget);

	/** Set the movement speed */
	UFUNCTION(BlueprintCallable, Category = "Tank Movement")
	void SetMoveSpeed(float NewSpeed);

	/** Set the stopping distance */
	UFUNCTION(BlueprintCallable, Category = "Tank Movement")
	void SetStoppingDistance(float NewDistance);

	/** Set the mesh rotation offset */
	UFUNCTION(BlueprintCallable, Category = "Tank Movement")
	void SetMeshRotation(float YawRotation);

	/** Set zigzag movement settings */
	UFUNCTION(BlueprintCallable, Category = "Tank Movement")
	void SetZigzagSettings(bool bEnableZigzag, float MinDistance, float MaxDistance);

	/** Get current move speed */
	UFUNCTION(BlueprintPure, Category = "Tank Movement")
	float GetMoveSpeed() const { return MoveSpeed; }

	/** Check if tank has reached the target */
	UFUNCTION(BlueprintPure, Category = "Tank Movement")
	bool HasReachedTarget() const;

private:
	/** Initial spawn position for calculating center angle */
	FVector InitialSpawnLocation;

	/** Center angle (direct line from spawn to base) in radians */
	float CenterAngleRad = 0.0f;

	/** Current zigzag direction: 1 = left (positive angle), -1 = right (negative angle) */
	int32 ZigzagDirection = 1;

	/** Current movement angle in radians (center angle +/- 45 degrees) */
	float CurrentMovementAngleRad = 0.0f;

	/** Distance remaining to travel on current zigzag leg */
	float RemainingZigzagDistance = 0.0f;

	/** Has the center line been crossed on current leg? */
	bool bHasCrossedCenter = false;

	/** Has zigzag movement been initialized? */
	bool bZigzagInitialized = false;

	/** Has a target been explicitly set? */
	bool bTargetSet = false;

	/** Move the tank toward target */
	void MoveTowardTarget(float DeltaTime);

	/** Move tank in zigzag pattern */
	void MoveZigzag(float DeltaTime);

	/** Smoothly rotate toward target */
	void RotateTowardTarget(float DeltaTime);

	/** Initialize zigzag movement parameters */
	void InitializeZigzagMovement();

	/** Update zigzag movement direction and calculate new leg */
	void UpdateZigzagDirection();

	/** Calculate the center angle from spawn to target */
	float CalculateCenterAngle() const;

	/** Check if tank has crossed the center line */
	bool HasCrossedCenterLine() const;

	/** Rotate toward the current zigzag movement angle */
	void RotateTowardZigzagAngle(float DeltaTime);
};
