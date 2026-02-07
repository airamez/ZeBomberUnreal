// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExplosionTypes.h"
#include "NiagaraSystem.h"
#include "ExplosionComponent.generated.h"

/**
 * Explosion component that can be attached to projectiles.
 * Allows easy configuration of explosion effects without modifying projectile code.
 * Simply set your explosion prefab in the Blueprint defaults.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZEBOMBER_API UExplosionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UExplosionComponent();

    /**
     * Spawn the configured explosion effect at the given location
     * @param Location - Where to spawn the explosion
     * @param Normal - Optional hit normal for orientation
     */
    UFUNCTION(BlueprintCallable, Category = "Explosion")
    void SpawnExplosion(const FVector& Location, const FVector& Normal = FVector::UpVector);

    /**
     * Spawn explosion using the projectile's current location
     */
    UFUNCTION(BlueprintCallable, Category = "Explosion")
    void SpawnExplosionAtOwner();

    /**
     * Quick spawn with just a particle system override
     */
    UFUNCTION(BlueprintCallable, Category = "Explosion")
    void SpawnExplosionSimple(UParticleSystem* ParticleSystem, UNiagaraSystem* NiagaraSystem = nullptr, float Scale = 1.0f);

    /** The explosion configuration to use when spawning */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion Config")
    FExplosionConfig ExplosionConfig;

protected:
    virtual void BeginPlay() override;
};
