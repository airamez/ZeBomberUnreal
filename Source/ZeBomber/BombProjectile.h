// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BombProjectile.generated.h"

UCLASS()
class ZEBOMBER_API ABombProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABombProjectile();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Collision sphere for hit detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomb")
	class USphereComponent* CollisionComponent;

	/** The mesh component for the bomb (assigned in Blueprint) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomb")
	class UStaticMeshComponent* BombMesh;

	/** Bomb speed parameter (additional speed on top of inherited velocity) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")
	float BombSpeed = 0.0f;

	/** Time in seconds before the bomb auto-destroys */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb", meta = (ClampMin = "1.0"))
	float LifeSpan = 15.0f;

	/** Explosion radius for splash damage (0 = direct hit only) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 300.0f;

	/** Mesh rotation offset to fix bomb model orientation (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bomb")
	FRotator MeshRotationOffset = FRotator(0.0f, 0.0f, 0.0f);

private:
	/** Called when the bomb hits something */
	UFUNCTION()
	void OnBombHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Called when the bomb overlaps something */
	UFUNCTION()
	void OnBombOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Check for tanks in explosion radius and destroy them */
	void DestroyTanksInRadius(const FVector& ExplosionLocation);
};
