// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "BomberPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class ZEBOMBER_API ABomberPawn : public APawn
{
	GENERATED_BODY()

public:
	ABomberPawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ==================== Components ====================

	/** Root scene component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomber")
	class USceneComponent* SceneRoot;

	/** Pivot for rotating the mesh without affecting flight direction */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomber")
	class USceneComponent* MeshPivot;

	/** The mesh component for the bomber (accepts static meshes) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomber")
	class UStaticMeshComponent* BomberMesh;

	/** Spring arm for camera follow */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraArm;

	/** Third person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// ==================== Camera Settings ====================

	/** Distance of camera behind/above the bomber */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "100.0"))
	float CameraDistance = 1500.0f;

	/** Camera pitch angle looking down at the bomber (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float CameraPitchAngle = -25.0f;

	/** Camera lag speed (lower = smoother/slower follow) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraLagSpeed = 3.0f;

	/** Camera rotation lag speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0.0"))
	float CameraRotationLagSpeed = 2.0f;

	// ==================== Flight Parameters ====================

	/** Current forward speed of the bomber (units/sec) */
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

	/** Pitch inertia - how slowly the bomber responds (0=instant, 0.95=heavy bomber) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float PitchInertia = 0.92f;

	/** Yaw rate (degrees/sec) - how fast the bomber turns left/right */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float YawRate = 15.0f;

	/** Yaw inertia - how slowly the bomber responds to turning (0=instant, 0.95=heavy bomber) */
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

	/** How quickly the bomber returns to level flight when no input (degrees/sec) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0"))
	float LevelingSpeed = 15.0f;

	/** Minimum flight altitude */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float MinAltitude = 500.0f;

	/** Starting altitude for the bomber */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	float StartAltitude = 5000.0f;

	/** Mesh rotation offset to fix model orientation (Pitch, Yaw, Roll) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
	FRotator MeshRotationOffset = FRotator(0.0f, 0.0f, 0.0f);

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

	/** Offset below bomber where bomb spawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bombing")
	FVector BombSpawnOffset = FVector(0.0f, 0.0f, -100.0f);

	// ==================== Enhanced Input ====================

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* BomberMappingContext;

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

private:
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

	/** Input handlers */
	void OnPitchDown(const FInputActionValue& Value);
	void OnPitchDownReleased(const FInputActionValue& Value);
	void OnPitchUp(const FInputActionValue& Value);
	void OnPitchUpReleased(const FInputActionValue& Value);
	void OnTurnLeft(const FInputActionValue& Value);
	void OnTurnLeftReleased(const FInputActionValue& Value);
	void OnTurnRight(const FInputActionValue& Value);
	void OnTurnRightReleased(const FInputActionValue& Value);
	void OnDropBomb(const FInputActionValue& Value);

	/** Flight update functions */
	void UpdateFlight(float DeltaTime);
	void DropBomb();
};
