// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeliWaveSpawner.h"
#include "HeliAI.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

AHeliWaveSpawner::AHeliWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AHeliWaveSpawner::BeginPlay()
{
	Super::BeginPlay();

	// Find the base target if not set
	if (!BaseTarget)
	{
		// Try to find actor with the church mesh by tag or name
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);

		for (AActor* Actor : FoundActors)
		{
			if (Actor && Actor->GetName().Contains("old_wooden_church"))
			{
				BaseTarget = Actor;
				break;
			}
		}

		// Fallback to world origin if no base found
		if (!BaseTarget)
			UE_LOG(LogTemp, Warning, TEXT("HeliWaveSpawner: No base target found! Helicopters will move to world origin."));
	}

	// Start first wave
	ScheduleNextWave();
}

void AHeliWaveSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHeliWaveSpawner::ScheduleNextWave()
{
	CurrentWave++;
	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Wave %d scheduled, spawning in %.1f seconds"), CurrentWave, WaveDelay);

	GetWorld()->GetTimerManager().SetTimer(WaveTimerHandle, this, &AHeliWaveSpawner::SpawnWave, WaveDelay, false);
}

void AHeliWaveSpawner::SpawnWave()
{
	if (!HeliClass)
	{
		UE_LOG(LogTemp, Error, TEXT("HeliWaveSpawner: No HeliClass set! Cannot spawn wave."));
		return;
	}

	// Calculate helicopters for this wave
	int32 HelisToSpawn = HelisPerWave + (CurrentWave - 1) * HelisAddedPerWave;

	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Spawning wave %d with %d helicopters"), CurrentWave, HelisToSpawn);

	UsedSpawnAngles.Empty();

	// Spawn all helicopters for this wave simultaneously
	for (int32 i = 0; i < HelisToSpawn; i++)
	{
		FVector SpawnLocation = GetRandomSpawnPosition();

		if (SpawnLocation.IsNearlyZero())
		{
			UE_LOG(LogTemp, Warning, TEXT("HeliWaveSpawner: Could not find valid spawn position for helicopter %d"), i);
			continue;
		}

		// Calculate rotation to face the base
		FVector TargetLocation = BaseTarget ? BaseTarget->GetActorLocation() : FVector::ZeroVector;
		FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
		SpawnRotation.Pitch = 0.0f;
		SpawnRotation.Roll = 0.0f;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APawn* SpawnedHeli = GetWorld()->SpawnActor<APawn>(HeliClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (SpawnedHeli)
		{
			// Assign random speed
			float RandomSpeed = FMath::FRandRange(MinHeliSpeed, MaxHeliSpeed);

			// If it's our HeliAI class, set the target, speed, stopping distance, mesh rotation, and fly height
			if (AHeliAI* HeliAI = Cast<AHeliAI>(SpawnedHeli))
			{
				HeliAI->SetMoveSpeed(RandomSpeed);
				HeliAI->SetStoppingDistance(LineOfFireDistance);
				HeliAI->SetMeshRotation(MeshRotationOffset);
				HeliAI->SetFlyHeight(SpawnLocation.Z); // Use the spawned height
				HeliAI->SetTargetLocation(TargetLocation);
			}

			// Bind to destruction event
			SpawnedHeli->OnDestroyed.AddDynamic(this, &AHeliWaveSpawner::OnHeliDestroyed);

			ActiveHeliCount++;

			UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Spawned helicopter %d/%d at %s with speed %.1f"),
				i + 1, HelisToSpawn, *SpawnLocation.ToString(), RandomSpeed);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Wave %d complete. Active helicopters: %d"), CurrentWave, ActiveHeliCount);
}

FVector AHeliWaveSpawner::GetRandomSpawnPosition()
{
	// Maximum attempts to find a valid position
	const int32 MaxAttempts = 50;
	const float AngleSeparationRad = FMath::DegreesToRadians(MinSpawnSeparation / SpawnRadius);

	for (int32 Attempt = 0; Attempt < MaxAttempts; Attempt++)
	{
		// Random angle between 0 and 360 degrees
		float RandomAngleDegrees = FMath::FRandRange(0.0f, 360.0f);
		float RandomAngleRad = FMath::DegreesToRadians(RandomAngleDegrees);

		// Check if this angle is far enough from all used angles
		bool bAngleValid = true;
		for (float UsedAngle : UsedSpawnAngles)
		{
			float AngleDiff = FMath::Abs(RandomAngleDegrees - UsedAngle);
			// Handle wrap-around at 360 degrees
			if (AngleDiff > 180.0f)
				AngleDiff = 360.0f - AngleDiff;

			float UsedAngleRad = FMath::DegreesToRadians(AngleDiff);
			if (UsedAngleRad < AngleSeparationRad)
			{
				bAngleValid = false;
				break;
			}
		}

		if (bAngleValid)
		{
			UsedSpawnAngles.Add(RandomAngleDegrees);

			// Convert polar coordinates to Cartesian
			float X = FMath::Cos(RandomAngleRad) * SpawnRadius;
			float Y = FMath::Sin(RandomAngleRad) * SpawnRadius;

			// Random height between MinSpawnHeight and MaxSpawnHeight
			float RandomHeight = FMath::FRandRange(MinSpawnHeight, MaxSpawnHeight);

			return FVector(X, Y, RandomHeight);
		}
	}

	// Could not find valid position
	UE_LOG(LogTemp, Warning, TEXT("HeliWaveSpawner: Could not find valid spawn position after %d attempts"), MaxAttempts);
	return FVector::ZeroVector;
}

void AHeliWaveSpawner::OnHeliDestroyed(AActor* DestroyedActor)
{
	ActiveHeliCount--;
	UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Helicopter destroyed. Active helicopters remaining: %d"), ActiveHeliCount);

	CheckWaveComplete();
}

void AHeliWaveSpawner::CheckWaveComplete()
{
	// If all helicopters from the wave are destroyed, start next wave
	if (ActiveHeliCount <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("HeliWaveSpawner: Wave %d complete!"), CurrentWave);
		ScheduleNextWave();
	}
}
