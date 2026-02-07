// Copyright Epic Games, Inc. All Rights Reserved.

#include "BombProjectile.h"
#include "TankAI.h"
#include "HeliAI.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ABombProjectile::ABombProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

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
		UE_LOG(LogTemp, Error, TEXT("BOMB BEGINPLAY: Initial rotation: %s"), *BeforeRot.ToString());
	}
}

void ABombProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	static bool LoggedOnce = false;
	if (!LoggedOnce && BombMesh)
	{
		LoggedOnce = true;
		FRotator CurrentRot = BombMesh->GetRelativeRotation();
		FVector CurrentLoc = BombMesh->GetRelativeLocation();
		UE_LOG(LogTemp, Error, TEXT("BOMB TICK: Rotation=%s Location=%s"), *CurrentRot.ToString(), *CurrentLoc.ToString());
	}
}

void ABombProjectile::OnBombHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Don't hit the bomber that dropped us
	if (OtherActor == GetOwner())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("BombProjectile: Hit %s"), *OtherActor->GetName());

	// Check if we directly hit a tank
	if (ATankAI* Tank = Cast<ATankAI>(OtherActor))
	{
		UE_LOG(LogTemp, Log, TEXT("BombProjectile: Direct hit on tank!"));
		Tank->Destroy();
	}

	// Check if we directly hit a helicopter
	if (AHeliAI* Heli = Cast<AHeliAI>(OtherActor))
	{
		UE_LOG(LogTemp, Log, TEXT("BombProjectile: Direct hit on helicopter!"));
		Heli->Destroy();
	}

	// Check for tanks and helicopters in explosion radius
	if (ExplosionRadius > 0.0f)
	{
		DestroyTanksInRadius(GetActorLocation());
		DestroyHelisInRadius(GetActorLocation());
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

void ABombProjectile::DestroyTanksInRadius(const FVector& ExplosionLocation)
{
	// Find all TankAI actors in the explosion radius
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATankAI::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (Actor && !Actor->IsPendingKillPending())
		{
			float Distance = FVector::Dist(ExplosionLocation, Actor->GetActorLocation());
			if (Distance <= ExplosionRadius)
			{
				UE_LOG(LogTemp, Log, TEXT("BombProjectile: Tank destroyed by explosion at distance %.0f"), Distance);
				Actor->Destroy();
			}
		}
	}
}

void ABombProjectile::DestroyHelisInRadius(const FVector& ExplosionLocation)
{
	// Find all HeliAI actors in the explosion radius
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AHeliAI::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (Actor && !Actor->IsPendingKillPending())
		{
			float Distance = FVector::Dist(ExplosionLocation, Actor->GetActorLocation());
			if (Distance <= ExplosionRadius)
			{
				UE_LOG(LogTemp, Log, TEXT("BombProjectile: Helicopter destroyed by explosion at distance %.0f"), Distance);
				Actor->Destroy();
			}
		}
	}
}
