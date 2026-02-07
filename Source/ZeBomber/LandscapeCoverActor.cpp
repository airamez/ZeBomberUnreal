// Copyright Epic Games, Inc. All Rights Reserved.

#include "LandscapeCoverActor.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "LandscapeComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ALandscapeCoverActor::ALandscapeCoverActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALandscapeCoverActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyMaterialsToLandscape();
}

ALandscapeProxy* ALandscapeCoverActor::FindLandscape() const
{
	for (TActorIterator<ALandscapeProxy> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	UE_LOG(LogTemp, Error, TEXT("LANDSCAPE COVER: No Landscape found in level!"));
	return nullptr;
}

void ALandscapeCoverActor::ApplyMaterialsToLandscape()
{
	if (!LandscapeSurface_1 && !LandscapeSurface_2 && !LandscapeSurface_3)
	{
		UE_LOG(LogTemp, Error, TEXT("LANDSCAPE COVER: No surfaces assigned!"));
		return;
	}

	UMaterialInterface* Surface1 = LandscapeSurface_1 ? LandscapeSurface_1 : (LandscapeSurface_2 ? LandscapeSurface_2 : LandscapeSurface_3);
	UMaterialInterface* Surface2 = LandscapeSurface_2 ? LandscapeSurface_2 : (LandscapeSurface_1 ? LandscapeSurface_1 : LandscapeSurface_3);
	UMaterialInterface* Surface3 = LandscapeSurface_3 ? LandscapeSurface_3 : (LandscapeSurface_2 ? LandscapeSurface_2 : LandscapeSurface_1);

	FVector2D Center2D(BaseCenter.X, BaseCenter.Y);
	float InnerRadiusSq = InnerRadius * InnerRadius;
	float OuterRadiusSq = OuterRadius * OuterRadius;

	int32 ProxyCount = 0;
	int32 Zone1Count = 0;
	int32 Zone2Count = 0;
	int32 Zone3Count = 0;

	for (TActorIterator<ALandscapeProxy> It(GetWorld()); It; ++It)
	{
		ALandscapeProxy* Proxy = *It;
		ProxyCount++;

		// Use the proxy's location to determine which zone it belongs to
		FVector ProxyLoc = Proxy->GetActorLocation();
		FVector2D ProxyLoc2D(ProxyLoc.X, ProxyLoc.Y);
		float DistSq = FVector2D::DistSquared(ProxyLoc2D, Center2D);

		UMaterialInterface* ChosenMaterial = nullptr;
		FString ZoneName;

		if (DistSq <= InnerRadiusSq)
		{
			ChosenMaterial = Surface1;
			ZoneName = TEXT("Zone1");
			Zone1Count++;
		}
		else if (DistSq <= OuterRadiusSq)
		{
			ChosenMaterial = Surface2;
			ZoneName = TEXT("Zone2");
			Zone2Count++;
		}
		else
		{
			ChosenMaterial = Surface3;
			ZoneName = TEXT("Zone3");
			Zone3Count++;
		}

		// Set LandscapeMaterial on the proxy — this is the correct way for landscapes
		Proxy->LandscapeMaterial = ChosenMaterial;

		// Force all components on this proxy to pick up the new material
		TArray<ULandscapeComponent*> Comps;
		Proxy->GetComponents<ULandscapeComponent>(Comps);
		for (ULandscapeComponent* Comp : Comps)
		{
			if (Comp)
			{
				Comp->UpdateMaterialInstances();
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("LANDSCAPE COVER: Proxy '%s' Dist=%.0f -> %s (%d comps)"),
			*Proxy->GetName(), FMath::Sqrt(DistSq), *ZoneName, Comps.Num());
	}

	UE_LOG(LogTemp, Warning, TEXT("LANDSCAPE COVER: Done. %d proxies | Zone1=%d Zone2=%d Zone3=%d | InnerR=%.0f OuterR=%.0f"),
		ProxyCount, Zone1Count, Zone2Count, Zone3Count, InnerRadius, OuterRadius);
}
