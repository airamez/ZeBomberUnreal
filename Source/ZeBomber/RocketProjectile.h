// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "RocketProjectile.generated.h"

UCLASS()
class ZEBOMBER_API ARocketProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARocketProjectile();

	/** Set the rocket's flight direction (called by BomberPawn on spawn) */
	void SetFlightDirection(const FVector& Direction);

	/** Projectile movement component handles flight */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rocket")
	UProjectileMovementComponent* ProjectileMovement;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Collision sphere for hit detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rocket")
	class USphereComponent* CollisionComponent;

	/** The mesh component for the rocket (assigned in Blueprint) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rocket")
	class UStaticMeshComponent* RocketMesh;

	/** Rocket speed (units/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket", meta = (ClampMin = "0.0"))
	float RocketSpeed = 8000.0f;

	/** Time in seconds before the rocket auto-destroys */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket", meta = (ClampMin = "1.0"))
	float LifeSpan = 10.0f;

	/** Explosion radius for splash damage (0 = direct hit only) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 200.0f;

	/** Mesh rotation offset to fix rocket model orientation (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket")
	FRotator MeshRotationOffset = FRotator(0.0f, 90.0f, 0.0f);

private:
	/** Called when the rocket hits something */
	UFUNCTION()
	void OnRocketHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Check for tanks in explosion radius and destroy them */
	void DestroyTanksInRadius(const FVector& ExplosionLocation);
};
