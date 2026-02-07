// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HeliAI.h"
#include "HeliWaveSpawner.generated.h"

UCLASS()
class ZEBOMBER_API AHeliWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	AHeliWaveSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** The class to spawn for helicopters (should be a Blueprint based on HeliAI) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning")
	TSubclassOf<class AHeliAI> HeliClass;

	/** The static mesh actor representing the base/center target */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Helicopter Spawning")
	AActor* BaseTarget;

	/** Distance from center (0,0,0) where helicopters spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "100.0"))
	float SpawnRadius = 2000.0f;

	/** Minimum helicopter speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float MinHeliSpeed = 200.0f;

	/** Maximum helicopter speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float MaxHeliSpeed = 500.0f;

	/** Number of helicopters to spawn in the first wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "1"))
	int32 HelisPerWave = 3;

	/** Additional helicopters added for each subsequent wave */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0"))
	int32 HelisAddedPerWave = 1;

	/** Time delay between waves in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float WaveDelay = 5.0f;

	/** Minimum distance between spawned helicopters */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "10.0"))
	float MinSpawnSeparation = 100.0f;

	/** Minimum height above ground to spawn helicopters */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float MinSpawnHeight = 400.0f;

	/** Maximum height above ground to spawn helicopters */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float MaxSpawnHeight = 800.0f;

	/** Rotation offset to fix helicopter model orientation (in degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning")
	float MeshRotationOffset = 90.0f;

	/** Distance from base where helicopters stop and start firing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (ClampMin = "0.0"))
	float LineOfFireDistance = 500.0f;

private:
	/** Current wave number */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (AllowPrivateAccess = "true"))
	int32 CurrentWave = 0;

	/** Number of active helicopters currently alive */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Helicopter Spawning", meta = (AllowPrivateAccess = "true"))
	int32 ActiveHeliCount = 0;

	/** Timer handle for wave spawning */
	FTimerHandle WaveTimerHandle;

	/** Set of used spawn angles to prevent overlapping */
	TArray<float> UsedSpawnAngles;

	/** Spawn a single wave of helicopters */
	UFUNCTION()
	void SpawnWave();

	/** Calculate a random spawn position on the circle that doesn't overlap with others */
	FVector GetRandomSpawnPosition();

	/** Called when a helicopter is destroyed */
	UFUNCTION()
	void OnHeliDestroyed(AActor* DestroyedActor);

	/** Start the next wave timer */
	void ScheduleNextWave();

	/** Check if all helicopters from current wave are destroyed and start next wave */
	void CheckWaveComplete();
};
