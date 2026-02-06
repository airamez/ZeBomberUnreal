// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TankWaveSpawner.generated.h"

UCLASS()
class ZEBOMBER_API ATankWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	ATankWaveSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** The skeletal mesh class to spawn for tanks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning")
	TSubclassOf<class APawn> TankClass;

	/** The static mesh actor representing the base/center target */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Tank Spawning")
	AActor* BaseTarget;

	/** Distance from center (0,0,0) where tanks spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "100.0"))
	float SpawnRadius = 2000.0f;

	/** Minimum tank speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "0.0"))
	float MinTankSpeed = 100.0f;

	/** Maximum tank speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "0.0"))
	float MaxTankSpeed = 300.0f;

	/** Number of tanks to spawn in the first wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "1"))
	int32 TanksPerWave = 5;

	/** Additional tanks added for each subsequent wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "0"))
	int32 TanksAddedPerWave = 2;

	/** Time delay between waves in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "0.0"))
	float WaveDelay = 5.0f;

	/** Minimum distance between spawned tanks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "10.0"))
	float MinSpawnSeparation = 100.0f;

	/** Height above ground to spawn tanks */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning")
	float SpawnHeightOffset = 100.0f;

	/** Rotation offset to fix tank model orientation (in degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning")
	float MeshRotationOffset = 90.0f;

	/** Distance from base where tanks stop and start firing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "0.0"))
	float LineOfFireDistance = 500.0f;

	/** Enable zigzag movement pattern (sailboat style) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning")
	bool bUseZigzagMovement = false;

	/** Minimum distance to travel after crossing center line before turning (zigzag) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "0.0"))
	float ZigzagMinDistance = 200.0f;

	/** Maximum distance to travel after crossing center line before turning (zigzag) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank Spawning", meta = (ClampMin = "0.0"))
	float ZigzagMaxDistance = 500.0f;

private:
	/** Current wave number */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tank Spawning", meta = (AllowPrivateAccess = "true"))
	int32 CurrentWave = 0;

	/** Number of active tanks currently alive */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tank Spawning", meta = (AllowPrivateAccess = "true"))
	int32 ActiveTankCount = 0;

	/** Timer handle for wave spawning */
	FTimerHandle WaveTimerHandle;

	/** Set of used spawn angles to prevent overlapping */
	TArray<float> UsedSpawnAngles;

	/** Spawn a single wave of tanks */
	UFUNCTION()
	void SpawnWave();

	/** Calculate a random spawn position on the circle that doesn't overlap with others */
	FVector GetRandomSpawnPosition();

	/** Called when a tank is destroyed */
	UFUNCTION()
	void OnTankDestroyed(AActor* DestroyedActor);

	/** Start the next wave timer */
	void ScheduleNextWave();

	/** Check if all tanks from current wave are destroyed and start next wave */
	void CheckWaveComplete();
};
