// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExplosionComponent.h"
#include "ExplosionEffect.h"

UExplosionComponent::UExplosionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UExplosionComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UExplosionComponent::SpawnExplosion(const FVector& Location, const FVector& Normal)
{
    AExplosionEffect::SpawnExplosion(this, Location, ExplosionConfig, Normal);
}

void UExplosionComponent::SpawnExplosionAtOwner()
{
    if (!GetOwner())
    {
        return;
    }

    FVector Location = GetOwner()->GetActorLocation();
    SpawnExplosion(Location);
}

void UExplosionComponent::SpawnExplosionSimple(UParticleSystem* ParticleSystem, UNiagaraSystem* NiagaraSystem, float Scale)
{
    FExplosionConfig Config;
    Config.ParticleSystem = ParticleSystem;
    Config.NiagaraSystem = NiagaraSystem;
    Config.ExplosionScale = Scale;
    Config.LifeSpan = 5.0f;

    if (GetOwner())
    {
        AExplosionEffect::SpawnExplosion(this, GetOwner()->GetActorLocation(), Config);
    }
}
