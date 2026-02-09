// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExplosionEffect.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerController.h"

AExplosionEffect::AExplosionEffect()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsInitialized = false;

    // Create root component
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = Root;

    // Create particle component (for Cascade systems)
    ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleComponent"));
    ParticleComponent->SetupAttachment(RootComponent);
    ParticleComponent->bAutoActivate = false;
    ParticleComponent->SetVisibleFlag(false); // Hide until valid asset is set

    // Create Niagara component (for Niagara systems)
    NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
    NiagaraComponent->SetupAttachment(RootComponent);
    NiagaraComponent->bAutoActivate = false;
    NiagaraComponent->SetVisibleFlag(false); // Hide until valid asset is set

    // Create audio component
    AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
    AudioComponent->SetupAttachment(RootComponent);
    AudioComponent->bAutoActivate = false;

    // Create decal component
    DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
    DecalComponent->SetupAttachment(RootComponent);
    DecalComponent->SetActive(false);
    DecalComponent->SetVisibleFlag(false); // Hide until valid decal is set
}

void AExplosionEffect::BeginPlay()
{
    Super::BeginPlay();

    if (!bIsInitialized)
    {
        ApplyConfiguration();
    }
}

void AExplosionEffect::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AExplosionEffect::InitializeExplosion(const FExplosionConfig& Config)
{
    CurrentConfig = Config;
    bIsInitialized = true;
    ApplyConfiguration();
}

AExplosionEffect* AExplosionEffect::SpawnExplosion(UObject* WorldContext, const FVector& Location, const FExplosionConfig& Config, const FVector& Normal)
{
    if (!WorldContext)
    {
        return nullptr;
    }

    UWorld* World = WorldContext->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    // Only spawn if there's something to show (prevent empty actor with debug visuals)
    if (!Config.ParticleSystem && !Config.NiagaraSystem && !Config.ExplosionSound && !Config.DecalMaterial)
    {
        UE_LOG(LogTemp, Log, TEXT("ExplosionEffect: No effect configured, skipping spawn"));
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Calculate rotation - add random rotation if enabled
    FRotator SpawnRotation = FRotator::ZeroRotator;
    if (Config.bRandomRotation)
    {
        SpawnRotation.Yaw = FMath::RandRange(0.0f, 360.0f);
    }

    // Apply location offset
    FVector FinalLocation = Location + Config.LocationOffset;

    AExplosionEffect* Explosion = World->SpawnActor<AExplosionEffect>(
        AExplosionEffect::StaticClass(),
        FinalLocation,
        SpawnRotation,
        SpawnParams);

    if (Explosion)
    {
        Explosion->InitializeExplosion(Config);
    }

    return Explosion;
}

AExplosionEffect* AExplosionEffect::SpawnExplosionSimple(
    UObject* WorldContext,
    const FVector& Location,
    UParticleSystem* ParticleSystem,
    UNiagaraSystem* NiagaraSystem,
    USoundBase* Sound,
    float Scale,
    float LifeSpan)
{
    FExplosionConfig Config;
    Config.ParticleSystem = ParticleSystem;
    Config.NiagaraSystem = NiagaraSystem;
    Config.ExplosionSound = Sound;
    Config.ExplosionScale = Scale;
    Config.LifeSpan = LifeSpan;

    return SpawnExplosion(WorldContext, Location, Config);
}

void AExplosionEffect::ApplyConfiguration()
{
    // Spawn particle or Niagara effect
    if (CurrentConfig.NiagaraSystem)
    {
        SpawnNiagaraEffect();
    }
    else if (CurrentConfig.ParticleSystem)
    {
        SpawnParticleEffect();
    }

    // Play sound
    if (CurrentConfig.ExplosionSound)
    {
        PlayExplosionSound();
    }

    // Apply scale
    SetActorScale3D(FVector(CurrentConfig.ExplosionScale));

    // Camera shake
    if (CurrentConfig.bShakeCamera)
    {
        ApplyCameraShake();
    }

    // Set cleanup timer
    SetLifeSpan(CurrentConfig.LifeSpan);
}

void AExplosionEffect::SpawnParticleEffect()
{
    if (!CurrentConfig.ParticleSystem)
    {
        return;
    }

    ParticleComponent->SetTemplate(CurrentConfig.ParticleSystem);
    ParticleComponent->SetVisibleFlag(true);
    ParticleComponent->ActivateSystem(true);
}

void AExplosionEffect::SpawnNiagaraEffect()
{
    if (!CurrentConfig.NiagaraSystem)
    {
        return;
    }

    NiagaraComponent->SetAsset(CurrentConfig.NiagaraSystem);
    NiagaraComponent->SetVisibleFlag(true);
    NiagaraComponent->Activate(true);
}

void AExplosionEffect::PlayExplosionSound()
{
    if (!CurrentConfig.ExplosionSound)
    {
        UE_LOG(LogTemp, Log, TEXT("ExplosionEffect: No explosion sound configured"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ExplosionEffect: Playing explosion sound %s at volume %.2f"), 
        *CurrentConfig.ExplosionSound->GetName(), CurrentConfig.SoundVolume);

    // Use UGameplayStatics for positional sound that attenuates naturally
    UGameplayStatics::PlaySoundAtLocation(
        this,
        CurrentConfig.ExplosionSound,
        GetActorLocation(),
        CurrentConfig.SoundVolume, // Use the volume from config instead of scale
        1.0f, // Pitch multiplier
        0.0f); // Start time
}

void AExplosionEffect::SpawnDecal(const FVector& Normal)
{
    if (!CurrentConfig.DecalMaterial)
    {
        return;
    }

    FVector DecalLocation = GetActorLocation() + Normal * 10.0f;
    FRotator DecalRotation = Normal.Rotation();
    DecalRotation.Pitch -= 90.0f; // Orient decal to face the surface

    float DecalSize = 200.0f * CurrentConfig.ExplosionScale;
    FVector DecalSize3D(DecalSize, DecalSize, DecalSize);

    // Spawn the decal using UGameplayStatics
    UGameplayStatics::SpawnDecalAtLocation(
        this,
        CurrentConfig.DecalMaterial,
        DecalSize3D,
        DecalLocation,
        DecalRotation,
        CurrentConfig.LifeSpan); // Fade out time
}

void AExplosionEffect::ApplyCameraShake()
{
    // Get all player controllers and shake if within range
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && PC->IsLocalController())
        {
            APawn* PlayerPawn = PC->GetPawn();
            if (PlayerPawn)
            {
                float Distance = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
                if (Distance <= CurrentConfig.CameraShakeRadius)
                {
                    // Scale shake intensity based on distance
                    float IntensityScale = 1.0f - (Distance / CurrentConfig.CameraShakeRadius);
                    float FinalIntensity = CurrentConfig.CameraShakeIntensity * IntensityScale;

                    // Simple camera shake using PC->ClientStartCameraShake would require a shake class
                    // For now, we'll log it - the user can add their own camera shake class
                    UE_LOG(LogTemp, Log, TEXT("ExplosionEffect: Camera shake at distance %.0f, intensity %.2f"), Distance, FinalIntensity);
                }
            }
        }
    }
}

void AExplosionEffect::CleanupExplosion()
{
    Destroy();
}
