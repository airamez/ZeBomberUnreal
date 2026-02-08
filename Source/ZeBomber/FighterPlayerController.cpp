// Copyright Epic Games, Inc. All Rights Reserved.

#include "FighterPlayerController.h"

AFighterPlayerController::AFighterPlayerController()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AFighterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Hide OS cursor completely - FighterPawn tracks a virtual cursor with zero lag
	bShowMouseCursor = false;

	// Game-only mode: raw mouse delta goes directly to input, no Slate processing
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	UE_LOG(LogTemp, Warning, TEXT("FighterPlayerController: BeginPlay - Game-only input, virtual cursor active"));
}
