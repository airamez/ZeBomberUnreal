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

	/** Get current move speed */
	UFUNCTION(BlueprintPure, Category = "Tank Movement")
	float GetMoveSpeed() const { return MoveSpeed; }

	/** Check if tank has reached the target */
	UFUNCTION(BlueprintPure, Category = "Tank Movement")
	bool HasReachedTarget() const;

private:
	/** Move the tank toward target */
	void MoveTowardTarget(float DeltaTime);

	/** Smoothly rotate toward target */
	void RotateTowardTarget(float DeltaTime);
};
