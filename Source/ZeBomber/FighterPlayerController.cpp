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

	// Hide OS cursor - FighterHUD draws custom crosshairs
	bShowMouseCursor = false;

	// GameAndUI mode: mouse position is tracked (for aiming) but OS cursor is hidden
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(true);
	SetInputMode(InputMode);

	UE_LOG(LogTemp, Warning, TEXT("FighterPlayerController: BeginPlay - Mouse hidden, HUD crosshair active"));
}
