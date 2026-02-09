// Copyright Epic Games, Inc. All Rights Reserved.

#include "RocketProjectile.h"
#include "TankAI.h"
#include "HeliAI.h"
#include "FighterPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

ARocketProjectile::ARocketProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create collision sphere as root
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(20.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->SetSimulatePhysics(false);
	CollisionComponent->SetEnableGravity(false);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	RootComponent = CollisionComponent;

	// Create static mesh for visual (assigned in Blueprint)
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create particle system component for trail
	TrailComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailComponent"));
	TrailComponent->SetupAttachment(RootComponent);

	// Projectile movement - handles flight with no gravity
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = RocketSpeed;
	ProjectileMovement->MaxSpeed = RocketSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // No gravity - flies straight
}

void ARocketProjectile::SetFlightDirection(const FVector& Direction)
{
	FVector Dir = Direction.GetSafeNormal();

	if (!Dir.IsNearlyZero())
	{
		// Use MakeRotFromX to avoid gimbal lock when firing straight up/down
		FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(Dir);
		SetActorRotation(NewRotation);
		ProjectileMovement->Velocity = Dir * RocketSpeed;
	}
}

void ARocketProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSpan);

	// Ensure physics is off - ProjectileMovement handles everything
	CollisionComponent->SetSimulatePhysics(false);
	CollisionComponent->SetEnableGravity(false);

	ProjectileMovement->InitialSpeed = RocketSpeed;
	ProjectileMovement->MaxSpeed = RocketSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	CollisionComponent->OnComponentHit.AddDynamic(this, &ARocketProjectile::OnRocketHit);

	// Apply mesh rotation offset
	if (RocketMesh)
	{
		RocketMesh->SetRelativeRotation(MeshRotationOffset);
	}

	// Set up trail - try Niagara first, then Cascade
	if (TrailNiagaraEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("RocketProjectile: TrailNiagaraEffect assigned: %s"), *TrailNiagaraEffect->GetName());
		UNiagaraFunctionLibrary::SpawnSystemAttached(TrailNiagaraEffect, RootComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
		UE_LOG(LogTemp, Warning, TEXT("RocketProjectile: Niagara trail activated"));
	}
	else if (TrailEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("RocketProjectile: TrailEffect assigned: %s"), *TrailEffect->GetName());
		if (TrailComponent)
		{
			TrailComponent->SetTemplate(TrailEffect);
			TrailComponent->ActivateSystem();
			UE_LOG(LogTemp, Warning, TEXT("RocketProjectile: Cascade trail activated"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RocketProjectile: No trail effect assigned"));
	}

	// Play fire sound when rocket spawns
	if (FireSound)
	{
		float Volume = 1.0f;
		if (AFighterPawn* Fighter = Cast<AFighterPawn>(GetOwner()))
		{
			Volume = Fighter->GetSoundVolume();
		}
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation(), Volume);
	}

	UE_LOG(LogTemp, Warning, TEXT("RocketProjectile: BeginPlay - Velocity=%s Speed=%.0f"),
		*ProjectileMovement->Velocity.ToString(), RocketSpeed);
}

void ARocketProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ARocketProjectile::OnRocketHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Don't hit the bomber that fired us
	if (OtherActor == GetOwner())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("RocketProjectile: Hit %s"), *OtherActor->GetName());

	// Check if we directly hit a helicopter
	if (AHeliAI* Heli = Cast<AHeliAI>(OtherActor))
	{
		UE_LOG(LogTemp, Log, TEXT("RocketProjectile: Direct hit on helicopter!"));
		Heli->Destroy();
	}
	else if (ATankAI* Tank = Cast<ATankAI>(OtherActor))
	{
		UE_LOG(LogTemp, Log, TEXT("RocketProjectile: Direct hit on tank - no effect"));
	}

	// Destroy the rocket
	Destroy();
}

