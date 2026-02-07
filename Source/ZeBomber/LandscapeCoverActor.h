// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LandscapeCoverActor.generated.h"

/**
 * Procedurally covers the entire Landscape with 3 radial material zones.
 * No manual Material Editor work required.
 *
 * Zone 1 (LandscapeSurface_1): From BaseCenter out to InnerRadius
 * Zone 2 (LandscapeSurface_2): From InnerRadius to OuterRadius
 * Zone 3 (LandscapeSurface_3): Beyond OuterRadius (the rest of the landscape)
 *
 * Each landscape component (tile) is assigned one of the 3 materials based on
 * the distance from its center to BaseCenter.
 *
 * Usage:
 *   1. Place this actor in your level.
 *   2. Assign 3 materials to LandscapeSurface_1/2/3
 *      (e.g. the Megascans MI_ assets from /Game/Fab/Megascans/Surfaces/...).
 *   3. Set BaseCenter to the world XY position of your base.
 *   4. Adjust InnerRadius and OuterRadius to control zone sizes.
 *   5. Hit Play. The landscape is covered automatically.
 */
UCLASS()
class ZEBOMBER_API ALandscapeCoverActor : public AActor
{
	GENERATED_BODY()

public:
	ALandscapeCoverActor();

protected:
	virtual void BeginPlay() override;

	// ==================== Surface Materials ====================

	/** Material for the inner zone (around the base). Assign a Megascans MI_ asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Cover")
	UMaterialInterface* LandscapeSurface_1;

	/** Material for the middle zone. Assign a Megascans MI_ asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Cover")
	UMaterialInterface* LandscapeSurface_2;

	/** Material for the outer zone (rest of the landscape). Assign a Megascans MI_ asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Cover")
	UMaterialInterface* LandscapeSurface_3;

	// ==================== Zone Configuration ====================

	/** World-space center point for the radial zones (only X and Y are used) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Cover")
	FVector BaseCenter = FVector::ZeroVector;

	/** Radius of the inner zone. Tiles closer than this get Surface_1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Cover", meta = (ClampMin = "0.0"))
	float InnerRadius = 10000.0f;

	/** Radius of the outer zone boundary. Tiles beyond this get Surface_3. Tiles between Inner and Outer get Surface_2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Cover", meta = (ClampMin = "0.0"))
	float OuterRadius = 30000.0f;

private:
	/** Find the landscape actor in the level */
	class ALandscapeProxy* FindLandscape() const;

	/** Assign materials to each landscape component based on distance from BaseCenter */
	void ApplyMaterialsToLandscape();
};
