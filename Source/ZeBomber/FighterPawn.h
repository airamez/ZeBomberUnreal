// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "FighterPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class UInputMappingContext;
class UInputAction;
class USoundBase;

/**
 * First-person fighter pawn. The camera sits at the nose of an invisible airplane.
 * WASD controls flight (A/D = yaw, W = tip down, S = tip up).
 * Mouse aims a white rocket crosshair; left-click fires rockets.
 * Space drops bombs; a red bomb-impact crosshair is projected on the ground.
 */
UCLASS()
class ZEBOMBER_API AFighterPawn : public APawn
{
	GENERATED_BODY()

public:
	AFighterPawn();

	/** Returns the predicted bomb impact location (used by HUD) */
	UFUNCTION(BlueprintCallable, Category = "Fighter")
	FVector GetBombImpactPoint() const { return BombImpactPoint; }

	/** Returns the mouse-aim world target (used by HUD) */
	UFUNCTION(BlueprintCallable, Category = "Fighter")
	FVector GetRocketAimPoint() const { return RocketAimWorldTarget; }

	/** Returns the virtual cursor screen position (used by HUD) */
	UFUNCTION(BlueprintCallable, Category = "Fighter")
	FVector2D GetCursorScreenPosition() const { return VirtualCursorPos; }

	/** Returns whether free-look is active (used by HUD) */
	UFUNCTION(BlueprintCallable, Category = "Fighter")
	bool IsFreeLookActive() const { return bFreeLookActive; }

	/** Returns current forward speed */
	UFUNCTION(BlueprintCallable, Category = "Fighter")
	float GetCurrentSpeed() const { return CurrentSpeed; }

	/** Returns current altitude (Z) */
	UFUNCTION(BlueprintCallable, Category = "Fighter")
	float GetCurrentAltitude() const { return GetActorLocation().Z; }

	/** Returns number of tanks destroyed */
	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetTanksDestroyed() const { return TanksDestroyed; }

	/** Returns number of helicopters destroyed */
	UFUNCTION(BlueprintCallable, Category = "Score")
	int32 GetHelisDestroyed() const { return HelisDestroyed; }

	/** Called by projectiles when they destroy an enemy */
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddTankKill() { TanksDestroyed++; }

	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddHeliKill() { HelisDestroyed++; }

	/** Returns current sound volume (0.0 - 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	float GetSoundVolume() const { return SoundVolume; }

	/** Returns current aim sensitivity */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	float GetAimSensitivity() const { return AimSensitivity; }

	/** Returns current radar zoom level (multiplier applied to radar world range) */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	float GetRadarZoom() const { return RadarZoom; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ==================== Components ====================

	/** Root scene component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fighter")
	class USceneComponent* SceneRoot;

	/** Pivot for free-look camera rotation (does not affect flight direction) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USceneComponent* CameraPivot;

	/** First-person camera at the nose of the airplane */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* NoseCamera;

	// ==================== Camera Settings ====================

	/** Camera position offset from the pawn origin (local space).
	 *  X = forward/back, Y = left/right, Z = up/down.
	 *  Tweak this to move the viewpoint along the airplane body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector CameraOffset = FVector(0.0f, 0.0f, 0.0f);

	/** Camera pitch offset in degrees (positive = look up, negative = look down).
	 *  Use this to tilt the default view slightly downward for better ground visibility. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float CameraPitchOffset = 0.0f;

	// ==================== Free-Look (Right Mouse Button) ====================

	/** Mouse sensitivity for free-look (degrees per pixel) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "0.01"))
	float FreeLookSensitivity = 0.15f;

	/** Maximum free-look yaw angle from center (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "1.0"))
	float FreeLookMaxYaw = 120.0f;

	/** Maximum free-look pitch angle from center (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "1.0"))
	float FreeLookMaxPitch = 80.0f;

	/** Speed at which the camera returns to forward after releasing RMB (higher = faster) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "0.5"))
	float FreeLookReturnSpeed = 5.0f;

	/** Mouse sensitivity for aiming crosshair (pixels per raw mouse unit) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "0.1"))
	float AimSensitivity = 2.5f;

	/** Free-look sensitivity multiplier (scales base AimSensitivity for free-look camera rotation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FreeLook", meta = (ClampMin = "0.1"))
	float FreeLookSensitivityMultiplier = 3.0f;

	// ==================== Flight Parameters ====================

	/** Current forward speed of the fighter (units/sec) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Flight")
	float CurrentSpeed = 0.0f;

	/** Minimum flight speed (stall speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float MinSpeed = 800.0f;

	/** Maximum flight speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float MaxSpeed = 3000.0f;

	/** Default cruising speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float DefaultSpeed = 1500.0f;

	/** Speed change rate when pitching (units/sec^2) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float SpeedChangeRate = 400.0f;

	/** Pitch rate (degrees/sec) - how fast the nose goes up/down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float PitchRate = 12.0f;

	/** Pitch inertia - how slowly the fighter responds (0=instant, 0.95=heavy) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float PitchInertia = 0.92f;

	/** Yaw rate (degrees/sec) - how fast the fighter turns left/right */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float YawRate = 15.0f;

	/** Yaw inertia - how slowly the fighter responds to turning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float YawInertia = 0.90f;

	/** Roll rate when turning (degrees/sec) - visual bank angle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float RollRate = 20.0f;

	/** Maximum roll angle when turning (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float MaxRollAngle = 30.0f;

	/** Maximum pitch angle (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float MaxPitchAngle = 45.0f;

	/** How quickly the fighter returns to level flight when no input (degrees/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float LevelingSpeed = 15.0f;

	/** Minimum flight altitude */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float MinAltitude = 500.0f;

	/** Starting altitude for the fighter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float StartAltitude = 5000.0f;

	// ==================== Rocket / Mouse Crosshair ====================

	/** Blueprint class for the rocket to fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket")
	TSubclassOf<AActor> RocketClass;

	/** Rocket fire rate cooldown (seconds between shots) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket", meta = (ClampMin = "0.01"))
	float RocketCooldown = 0.15f;

	/** Offset from pawn origin where rockets spawn (local space).
	 *  Positive X = forward, negative Z = below camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket")
	FVector RocketSpawnOffset = FVector(300.0f, 0.0f, -50.0f);

	/** Maximum distance for mouse-aim raycast (units) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rocket", meta = (ClampMin = "1000.0"))
	float CrosshairMaxDistance = 50000.0f;

	// ==================== Bombing ====================

	/** Blueprint class for the bomb to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bombing")
	TSubclassOf<AActor> BombClass;

	/** Additional speed added to the bomb on drop (units/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bombing", meta = (ClampMin = "0.0"))
	float BombDropSpeed = 0.0f;

	/** Cooldown between bomb drops (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bombing", meta = (ClampMin = "0.0"))
	float BombCooldown = 0.5f;

	/** Offset below the fighter where bomb spawns (local space) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bombing")
	FVector BombSpawnOffset = FVector(0.0f, 0.0f, -100.0f);

	/** Gravity used for bomb impact prediction (positive value, units/sec^2).
	 *  Should match the physics gravity magnitude in Project Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bombing", meta = (ClampMin = "1.0"))
	float BombGravity = 980.0f;

	/** Sound to play when a bomb is released */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bombing")
	USoundBase* BombDropSound;

	// ==================== Enhanced Input ====================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* FighterMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PitchDownAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PitchUpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* TurnLeftAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* TurnRightAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* DropBombAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* FireRocketAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* FreeLookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* VolumeUpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* VolumeDownAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SensitivityUpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SensitivityDownAction;

private:
	// ==================== Internal State ====================

	/** Current pitch input (-1 to 1) */
	float PitchInput = 0.0f;

	/** Smoothed pitch input for inertia */
	float SmoothedPitchInput = 0.0f;

	/** Current yaw input (-1 to 1) */
	float YawInput = 0.0f;

	/** Smoothed yaw input for inertia */
	float SmoothedYawInput = 0.0f;

	/** Time when last bomb was dropped */
	float LastBombDropTime = -999.0f;

	/** Time when last rocket was fired */
	float LastRocketFireTime = -999.0f;

	/** Whether fire button is held */
	bool bFireRocketHeld = false;

	/** Warmup complete flag - prevents input on first frames */
	bool bWarmupComplete = false;

	/** Warmup countdown timer */
	float WarmupTimer = 1.0f;

	/** Current mouse-aim world target location (for rockets / white crosshair) */
	FVector RocketAimWorldTarget = FVector::ZeroVector;

	/** Predicted bomb impact point on the ground (for red crosshair) */
	FVector BombImpactPoint = FVector::ZeroVector;

	/** Whether the bomb impact prediction is valid (hits ground) */
	bool bBombImpactValid = false;

	/** Score tracking */
	int32 TanksDestroyed = 0;
	int32 HelisDestroyed = 0;

	/** Sound volume (0.0 - 1.0) */
	float SoundVolume = 0.5f;

	/** Volume/sensitivity step per key press */
	float VolumeStep = 0.05f;
	float SensitivityStep = 0.1f;
	float MinSensitivity = 0.1f;
	float MaxSensitivity = 5.0f;

	/** Radar zoom level (1.0 = default, lower = zoomed in, higher = zoomed out) */
	float RadarZoom = 1.0f;
	float RadarZoomStep = 0.15f;
	float RadarZoomMin = 0.2f;
	float RadarZoomMax = 3.0f;

	/** Timer for periodic enemy scanning */
	float EnemyScanTimer = 0.0f;

	/** Set of enemies we've already bound OnDestroyed to (avoid double-binding) */
	TSet<AActor*> BoundEnemies;

	/** Cached mouse delta for current frame (read once, used by cursor + free-look) */
	float FrameMouseDeltaX = 0.0f;
	float FrameMouseDeltaY = 0.0f;

	/** Virtual cursor position on screen (replaces OS cursor for zero-lag) */
	FVector2D VirtualCursorPos = FVector2D::ZeroVector;

	/** Whether free-look (RMB) is currently active */
	bool bFreeLookActive = false;

	/** Current free-look rotation offset from default camera orientation */
	FRotator FreeLookRotation = FRotator::ZeroRotator;

	// ==================== Input Handlers ====================

	void OnPitchDown(const FInputActionValue& Value);
	void OnPitchDownReleased(const FInputActionValue& Value);
	void OnPitchUp(const FInputActionValue& Value);
	void OnPitchUpReleased(const FInputActionValue& Value);
	void OnTurnLeft(const FInputActionValue& Value);
	void OnTurnLeftReleased(const FInputActionValue& Value);
	void OnTurnRight(const FInputActionValue& Value);
	void OnTurnRightReleased(const FInputActionValue& Value);
	void OnDropBomb(const FInputActionValue& Value);
	void OnFireRocket(const FInputActionValue& Value);
	void OnFireRocketReleased(const FInputActionValue& Value);
	void OnFreeLookPressed(const FInputActionValue& Value);
	void OnFreeLookReleased(const FInputActionValue& Value);
	void OnVolumeUp(const FInputActionValue& Value);
	void OnVolumeDown(const FInputActionValue& Value);
	void OnSensitivityUp(const FInputActionValue& Value);
	void OnSensitivityDown(const FInputActionValue& Value);

	// ==================== Core Logic ====================

	void UpdateFlight(float DeltaTime);
	void UpdateVirtualCursor(float DeltaTime);
	void UpdateFreeLook(float DeltaTime);
	void UpdateMouseAim();
	void UpdateBombImpactPrediction();
	void DropBomb();
	void FireRocket();
	void BindEnemyDestroyedEvents();

	UFUNCTION()
	void OnEnemyDestroyed(AActor* DestroyedActor);
};
