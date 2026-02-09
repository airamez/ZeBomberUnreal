// Copyright Epic Games, Inc. All Rights Reserved.

#include "BombProjectile.h"
#include "TankAI.h"
#include "HeliAI.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ABombProjectile::ABombProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create collision sphere as root
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(50.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->SetSimulatePhysics(true);
	CollisionComponent->SetEnableGravity(true);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	RootComponent = CollisionComponent;

	// Create static mesh - NOT attached to physics component
	BombMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BombMesh"));
	BombMesh->SetupAttachment(RootComponent); // Attach to root, not collision
	BombMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABombProjectile::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSpan);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ABombProjectile::OnBombHit);

	if (BombMesh)
	{
		FRotator BeforeRot = BombMesh->GetRelativeRotation();
		UE_LOG(LogTemp, Log, TEXT("BombProjectile: BeginPlay rotation: %s"), *BeforeRot.ToString());
	}
}

void ABombProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABombProjectile::OnBombHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Don't hit the bomber that dropped us
	if (OtherActor == GetOwner())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("BombProjectile: Hit %s at %s"), *OtherActor->GetName(), *GetActorLocation().ToString());

	// Splash damage: find all actors within ExplosionRadius
	FVector BombLocation = GetActorLocation();

	if (ExplosionRadius > 0.0f)
	{
		// Use sphere trace to find all actors in blast radius
		TArray<FHitResult> HitResults;
		FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
		bool bHasHits = GetWorld()->SweepMultiByChannel(
			HitResults,
			BombLocation,
			BombLocation,
			FQuat::Identity,
			ECC_WorldDynamic,
			Sphere
		);

		if (bHasHits)
		{
			for (const FHitResult& Hit : HitResults)
			{
				AActor* HitActor = Hit.GetActor();
				if (!HitActor || HitActor == this || HitActor == GetOwner())
				{
					continue;
				}

				if (ATankAI* Tank = Cast<ATankAI>(HitActor))
				{
					float Dist = FVector::Dist(BombLocation, Tank->GetActorLocation());
					UE_LOG(LogTemp, Log, TEXT("BombProjectile: Splash hit tank at distance %.0f (radius %.0f)"), Dist, ExplosionRadius);
					Tank->Destroy();
				}
				else if (AHeliAI* Heli = Cast<AHeliAI>(HitActor))
				{
					float Dist = FVector::Dist(BombLocation, Heli->GetActorLocation());
					UE_LOG(LogTemp, Log, TEXT("BombProjectile: Splash hit heli at distance %.0f (radius %.0f)"), Dist, ExplosionRadius);
					Heli->Destroy();
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("BombProjectile: No splash hits found within radius %.0f"), ExplosionRadius);
		}
	}

	// Also check direct hit (in case overlap missed due to collision channel)
	if (ATankAI* Tank = Cast<ATankAI>(OtherActor))
	{
		if (!Tank->IsActorBeingDestroyed())
		{
			UE_LOG(LogTemp, Log, TEXT("BombProjectile: Direct hit on tank!"));
			Tank->Destroy();
		}
	}
	else if (AHeliAI* Heli = Cast<AHeliAI>(OtherActor))
	{
		if (!Heli->IsActorBeingDestroyed())
		{
			UE_LOG(LogTemp, Log, TEXT("BombProjectile: Direct hit on helicopter!"));
			Heli->Destroy();
		}
	}

	// Destroy the bomb
	Destroy();
}

void ABombProjectile::OnBombOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Don't hit the bomber that dropped us
	if (OtherActor == GetOwner())
	{
		return;
	}

	// Check if we hit a tank
	if (ATankAI* Tank = Cast<ATankAI>(OtherActor))
	{
		UE_LOG(LogTemp, Log, TEXT("BombProjectile: Overlap hit on tank!"));
		Tank->Destroy();
		Destroy();
	}

	// Check if we hit a helicopter
	if (AHeliAI* Heli = Cast<AHeliAI>(OtherActor))
	{
		UE_LOG(LogTemp, Log, TEXT("BombProjectile: Overlap hit on helicopter!"));
		Heli->Destroy();
		Destroy();
	}
}

