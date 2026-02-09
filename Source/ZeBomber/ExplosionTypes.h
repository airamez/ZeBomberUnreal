// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraSystem.h"
#include "ExplosionTypes.generated.h"

/**
 * Configuration for an explosion effect
 * Allows customization of particle system, sound, decal, and damage
 */
USTRUCT(BlueprintType)
struct FExplosionConfig
{
    GENERATED_BODY()

    /** Display name for this explosion type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    FName DisplayName = TEXT("Default Explosion");

    /** The particle system to spawn (Niagara or Cascade from Fire_EXP_Vol01_Free) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    UParticleSystem* ParticleSystem = nullptr;

    /** Optional: Niagara system for the explosion (modern alternative to Cascade) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    UNiagaraSystem* NiagaraSystem = nullptr;

    /** Sound effect to play on explosion */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    USoundBase* ExplosionSound = nullptr;

    /** Volume multiplier for explosion sound (0.0 - 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SoundVolume = 1.0f;

    /** Optional: Decal to project on the ground (scorch mark) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    UMaterialInterface* DecalMaterial = nullptr;

    /** Scale of the explosion effect */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float ExplosionScale = 1.0f;

    /** Duration before the explosion actor destroys itself (after effects finish) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (ClampMin = "0.1", ClampMax = "30.0"))
    float LifeSpan = 5.0f;

    /** Shake the camera when exploding */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    bool bShakeCamera = true;

    /** Camera shake intensity (if bShakeCamera is true) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (ClampMin = "0.0", ClampMax = "10.0", EditCondition = "bShakeCamera"))
    float CameraShakeIntensity = 1.0f;

    /** Camera shake radius - players within this distance get shaken */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (ClampMin = "0.0", ClampMax = "10000.0", EditCondition = "bShakeCamera"))
    float CameraShakeRadius = 2000.0f;

    /** Random rotation offset for variety */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    bool bRandomRotation = true;

    /** Offset from hit location */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    FVector LocationOffset = FVector::ZeroVector;

    FExplosionConfig()
    {
        DisplayName = TEXT("Default Explosion");
        ParticleSystem = nullptr;
        NiagaraSystem = nullptr;
        ExplosionSound = nullptr;
        SoundVolume = 1.0f;
        DecalMaterial = nullptr;
        ExplosionScale = 1.0f;
        LifeSpan = 5.0f;
        bShakeCamera = true;
        CameraShakeIntensity = 1.0f;
        CameraShakeRadius = 2000.0f;
        bRandomRotation = true;
        LocationOffset = FVector::ZeroVector;
    }
};
