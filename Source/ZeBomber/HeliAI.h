// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ExplosionComponent.h"
#include "HeliAI.generated.h"

UCLASS()
class ZEBOMBER_API AHeliAI : public APawn
{
	GENERATED_BODY()

public:
	/** Explosion component for death effects */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Helicopter")
	UExplosionComponent* ExplosionComp;

	AHeliAI();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The static mesh component for the helicopter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Helicopter")
	class UStaticMeshComponent* HeliMesh;

	/** The root scene component (for actor rotation) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Helicopter")
	class USceneComponent* SceneRoot;

	/** The target location to move toward (base/church position) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Movement")
	FVector TargetLocation;

	/** Movement speed in units per second */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Movement")
	float MoveSpeed = 300.0f;

	/** Distance to stop from target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Movement", meta = (ClampMin = "0.0"))
	float StoppingDistance = 100.0f;

	/** Rotation offset to fix helicopter model orientation (in degrees) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Movement")
	float MeshRotationOffset = 0.0f;

	/** Rotation speed when turning toward target */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Movement", meta = (ClampMin = "0.0"))
	float RotationSpeed = 3.0f;

	/** Flying height for the helicopter */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Movement")
	float FlyHeight = 500.0f;

public:
	/** Set the target location for the helicopter to move toward */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetTargetLocation(const FVector& NewTarget);

	/** Set the movement speed */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetMoveSpeed(float NewSpeed);

	/** Set the stopping distance */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetStoppingDistance(float NewDistance);

	/** Set the mesh rotation offset */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetMeshRotation(float YawRotation);

	/** Set the flying height */
	UFUNCTION(BlueprintCallable, Category = "Helicopter Movement")
	void SetFlyHeight(float NewHeight);

	/** Get current move speed */
	UFUNCTION(BlueprintPure, Category = "Helicopter Movement")
	float GetMoveSpeed() const { return MoveSpeed; }

	/** Check if helicopter has reached the target */
	UFUNCTION(BlueprintPure, Category = "Helicopter Movement")
	bool HasReachedTarget() const;

private:
	/** Initial spawn position for reference */
	FVector InitialSpawnLocation;

	/** Has a target been explicitly set? */
	bool bTargetSet = false;

	/** Move the helicopter toward target */
	void MoveTowardTarget(float DeltaTime);

	/** Smoothly rotate toward target */
	void RotateTowardTarget(float DeltaTime);
};
