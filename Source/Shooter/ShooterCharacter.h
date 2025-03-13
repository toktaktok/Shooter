// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AmmoType.h"
#include "ShooterCharacter.generated.h"

class AWeapon;
class AItem;

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_FireTimerInProgress UMETA(DisplayName = "FireTimerInProgress"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),

	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()

class SHOOTER_API AShooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AShooterCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	//Getters
	UFUNCTION(BlueprintCallable)
	float			GetCrosshairSpreadMultiplier() const;

	class USpringArmComponent* GetCameraBoom()const { return SpringArmComp; }
	class UCameraComponent* GetCamera()const { return CameraComp; }

	int8			GetOverlappedItemCount() const { return OverlappedItemCount; }
	ECombatState	GetCombatState() const { return CombatState; }
	FVector			GetCameraInterpLocation();
	void			GetPickupItem(AItem* item);


	bool IsAiming() const { return bAiming; }
	bool IsCrouching() const { return bCrouching; }
	//오버랩된 아이템 카운트가 몇 개인지 추적한다.
	void IncrementOverlappedItemCount(int8 Amount);

protected:
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);

	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);

	void TurnByMouse(float Rate);
	void LookUpByMouse(float Rate);

	void FireWeapon();

	void AimingButtonPressed();
	void AimingButtonReleased();

	void FireButtonPressed();
	void FireButtonReleased();

	void SelectButtonPressed();
	void SelectButtonReleased();

	void CrouchButtonPressed();

	void SwapWeapon(AWeapon* WeaponToSwap);

	void StartFireTimer();

	void TraceForItems();

	UFUNCTION()
	void ResetAutoFire();

	bool GetBeamEndLocation(const FVector& MuzzleSocketLoc, FVector& OutBeamLoc);

	bool TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation);

	// Weapon

	AWeapon* SpawnDefaultWeapon();

	void EquipWeapon(AWeapon* EquippedWeapon);

	void DropWeapon();

	void InitializeAmmoMap();
	bool WeaponHasAmmo();

private:
	void CalculateFOV(float DeltaTime);
	void CalculateAimingSensivity();

	void CalculateCrosshairSpread(float DeltaTime);

	UFUNCTION()
	void StartCrosshairBulletFire();

	UFUNCTION()
	void FinishCrosshairBulletFire();

	FVector2D MakeRaisedCrosshairLocation() const;

	void PlayFireSound() const;
	void SendBullet();
	void PlayGunfireMontage();
	void SpawnMuzzleFX() const;
	void SpawnImpactFX(const FVector& SpawnLocation, const FTransform& Transform) const;

	void ReloadButtonPressed();
	void ReloadWeapon();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	bool CarryingAmmo();

	UFUNCTION(BlueprintCallable)
	void GrabClip();
	UFUNCTION(BlueprintCallable)
	void ReleaseClip();



	/* Camera Settings */

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	float CamDefaultFOV{};

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	float CamZoomedFOV{ 35.f };

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	float CamCurrentFOV{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float ZoomInterpSpeed{ 20.f };

	UPROPERTY(EditAnywhere, Category = "Camera")
	float MoveSpeed{ 100.f };

	UPROPERTY(EditAnywhere, Category = "Camera")
	float BaseTurnRate{ 45.f };

	UPROPERTY(EditAnywhere, Category = "Camera")
	float BaseLookUpRate{ 45.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	float CameraInterpDistance{ 250.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	float CameraInterpElevation{ 65.f };


	/* Crosshair Settings */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float CrosshairSpreadMultiplier{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float CrosshairVelocityFactor{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float CrosshairInAirFactor{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float CrosshairAimFactor{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float CrosshairShootFactor{};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float ShootTimeDuration{ .05f };

	bool bFiringBullet{ false };
	FTimerHandle CrosshairShootTimer;


	/* 자동 사격 */

	bool bFireButtonPressed{ false };
	bool bShouldFire{ true };
	float AutomaticFireRate{ .1f };
	FTimerHandle AutoFireTimer;

	bool bShouldTraceForItems{ false };
	int8 OverlappedItemCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	class AItem* TraceHitItemLastFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	AWeapon* EquippedWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AWeapon> DefaultWeaponClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	AItem* TraceHitItem{ nullptr };



#pragma region Rate Settings for Aiming and not (controller)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float AimingMouseSensibity{ .5f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float HipTurnRate{ 90.f };
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float HipLookUpRate{ 90.f };
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float AimingTurnRate{ 20.f };
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float AimingLookUpRate{ 20.f };

#pragma endregion


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ShotDistance{ 10000.f };


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* HipFireMontage;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* CameraComp;

#pragma region FX & Sound

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class USoundCue* FireSound;

#pragma endregion

#pragma region Bullet & Reload

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	TMap<EAmmoType, int32> AmmoMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	int32 Starting9mmAmmo = { 85 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	int32 StartingARAmmo = { 120 };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	ECombatState CombatState = { ECombatState::ECS_Unoccupied };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ReloadMontage;

#pragma endregion

#pragma region Clip

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	FTransform ClipTransform;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	USceneComponent* HandSceneComponent;

#pragma endregion

#pragma region Crouch
	bool bCrouching{ false };
#pragma endregion
};
