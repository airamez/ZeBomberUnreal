// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExplosionComponent.h"
#include "ExplosionEffect.h"
#include "FighterPawn.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

UExplosionComponent::UExplosionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UExplosionComponent::BeginPlay()
{
    Super::BeginPlay();
}

float UExplosionComponent::GetSoundVolume() const
{
    // First try to get volume from owner if it's a FighterPawn
    if (AFighterPawn* Fighter = Cast<AFighterPawn>(GetOwner()))
    {
        UE_LOG(LogTemp, Log, TEXT("ExplosionComponent: Got volume %.2f from FighterPawn owner"), Fighter->GetSoundVolume());
        return Fighter->GetSoundVolume();
    }
    
    // If owner is not a FighterPawn, try to find the player's FighterPawn in the world
    if (GetWorld())
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (APlayerController* PC = It->Get())
            {
                if (AFighterPawn* Fighter = Cast<AFighterPawn>(PC->GetPawn()))
                {
                    UE_LOG(LogTemp, Log, TEXT("ExplosionComponent: Got volume %.2f from world FighterPawn"), Fighter->GetSoundVolume());
                    return Fighter->GetSoundVolume();
                }
            }
        }
    }
    
    // Default volume if no FighterPawn found
    UE_LOG(LogTemp, Log, TEXT("ExplosionComponent: No FighterPawn found, using default volume 1.0"));
    return 1.0f;
}

void UExplosionComponent::SpawnExplosion(const FVector& Location, const FVector& Normal)
{
    FExplosionConfig Config = ExplosionConfig;
    Config.SoundVolume = GetSoundVolume();
    
    UE_LOG(LogTemp, Log, TEXT("ExplosionComponent: Spawning explosion with sound %s at volume %.2f"), 
        Config.ExplosionSound ? *Config.ExplosionSound->GetName() : TEXT("None"), 
        Config.SoundVolume);
    
    AExplosionEffect::SpawnExplosion(this, Location, Config, Normal);
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
    Config.SoundVolume = GetSoundVolume();

    if (GetOwner())
    {
        AExplosionEffect::SpawnExplosion(this, GetOwner()->GetActorLocation(), Config);
    }
}
