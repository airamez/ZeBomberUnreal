// Copyright Epic Games, Inc. All Rights Reserved.

#include "RocketProjectile.h"
#include "TankAI.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

ARocketProjectile::ARocketProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

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

	// Projectile movement - handles flight with no gravity
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = RocketSpeed;
	ProjectileMovement->MaxSpeed = RocketSpeed;
	ProjectileMovement->bRotationFollowsVelocity = false;
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

	// Check if we directly hit a tank
	if (ATankAI* Tank = Cast<ATankAI>(OtherActor))
	{
		UE_LOG(LogTemp, Log, TEXT("RocketProjectile: Direct hit on tank!"));
		Tank->Destroy();
	}

	// Check for tanks in explosion radius
	if (ExplosionRadius > 0.0f)
	{
		DestroyTanksInRadius(GetActorLocation());
	}

	// Destroy the rocket
	Destroy();
}

void ARocketProjectile::DestroyTanksInRadius(const FVector& ExplosionLocation)
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATankAI::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (Actor && !Actor->IsPendingKillPending())
		{
			float Distance = FVector::Dist(ExplosionLocation, Actor->GetActorLocation());
			if (Distance <= ExplosionRadius)
			{
				UE_LOG(LogTemp, Log, TEXT("RocketProjectile: Tank destroyed by explosion at distance %.0f"), Distance);
				Actor->Destroy();
			}
		}
	}
}
