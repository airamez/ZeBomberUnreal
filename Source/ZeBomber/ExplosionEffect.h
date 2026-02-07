// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExplosionTypes.h"
#include "NiagaraComponent.h"
#include "ExplosionEffect.generated.h"

class UParticleSystemComponent;
class UNiagaraComponent;
class UDecalComponent;
class UAudioComponent;

/**
 * Explosion effect actor that can be spawned at hit locations.
 * Supports both Cascade particle systems and Niagara systems from the Fire_EXP package.
 * Provides customization options for scale, sound, decals, and camera shake.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZEBOMBER_API AExplosionEffect : public AActor
{
    GENERATED_BODY()

public:
    AExplosionEffect();

    /**
     * Initialize and spawn the explosion effect with the given configuration
     * @param Config - The explosion configuration to use
     */
    UFUNCTION(BlueprintCallable, Category = "Explosion")
    void InitializeExplosion(const FExplosionConfig& Config);

    /**
     * Static helper to spawn an explosion at a location
     * @param World - The world context
     * @param Location - Where to spawn the explosion
     * @param Config - The explosion configuration
     * @param Normal - Optional hit normal for orienting the explosion
     * @return The spawned explosion actor
     */
    UFUNCTION(BlueprintCallable, Category = "Explosion", meta = (WorldContext = "World", UnsafeDuringActorConstruction = "true"))
    static AExplosionEffect* SpawnExplosion(UObject* WorldContext, const FVector& Location, const FExplosionConfig& Config, const FVector& Normal = FVector::UpVector);

    /**
     * Static helper to spawn an explosion at a location with simplified parameters
     * Uses default config with overrides
     */
    UFUNCTION(BlueprintCallable, Category = "Explosion", meta = (WorldContext = "World", UnsafeDuringActorConstruction = "true"))
    static AExplosionEffect* SpawnExplosionSimple(
        UObject* WorldContext,
        const FVector& Location,
        UParticleSystem* ParticleSystem,
        UNiagaraSystem* NiagaraSystem = nullptr,
        USoundBase* Sound = nullptr,
        float Scale = 1.0f,
        float LifeSpan = 5.0f);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** The particle system component (for Cascade systems) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explosion")
    UParticleSystemComponent* ParticleComponent;

    /** The Niagara component (for Niagara systems from Fire_EXP package) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explosion")
    UNiagaraComponent* NiagaraComponent;

    /** Audio component for explosion sound */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explosion")
    UAudioComponent* AudioComponent;

    /** Optional decal component for scorch marks */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explosion")
    UDecalComponent* DecalComponent;

    /** The explosion configuration used for this instance */
    UPROPERTY(BlueprintReadOnly, Category = "Explosion")
    FExplosionConfig CurrentConfig;

    /** Whether the explosion has been initialized */
    UPROPERTY(BlueprintReadOnly, Category = "Explosion")
    bool bIsInitialized;

    /** Timer handle for cleanup */
    FTimerHandle CleanupTimerHandle;

    /** Apply the explosion configuration */
    void ApplyConfiguration();

    /** Spawn the particle effect */
    void SpawnParticleEffect();

    /** Spawn the Niagara effect */
    void SpawnNiagaraEffect();

    /** Play explosion sound */
    void PlayExplosionSound();

    /** Spawn ground decal */
    void SpawnDecal(const FVector& Normal);

    /** Apply camera shake to nearby players */
    void ApplyCameraShake();

    /** Cleanup and destroy the actor */
    UFUNCTION()
    void CleanupExplosion();
};
