// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FighterPlayerController.generated.h"

/**
 * Player controller for the Fighter pawn.
 * Configures mouse input so the cursor is visible and confined to the viewport
 * for precise rocket aiming, while keyboard input drives the airplane.
 */
UCLASS()
class ZEBOMBER_API AFighterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFighterPlayerController();

protected:
	virtual void BeginPlay() override;
};
