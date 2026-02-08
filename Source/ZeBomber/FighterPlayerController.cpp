// Copyright Epic Games, Inc. All Rights Reserved.

#include "FighterPlayerController.h"

AFighterPlayerController::AFighterPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AFighterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Show cursor and confine to viewport for precise aiming
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	UE_LOG(LogTemp, Warning, TEXT("FighterPlayerController: BeginPlay - Mouse configured for aiming"));
}
