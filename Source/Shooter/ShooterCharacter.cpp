#include "ShooterCharacter.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMeshSocket.h"

#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Camera/CameraComponent.h"
#include "Sound/SoundCue.h"

#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

#include "Weapon.h"
#include "CustomUtils.h"

AShooterCharacter::AShooterCharacter() :
	BaseTurnRate{ 45.f },
	BaseLookUpRate{ 45.f },
	bAiming{ false }
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength = 180.f;
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SocketOffset = { 0.f, 50.f, 70.f };

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	//캐릭터가 입력 방향으로 움직인다
	GetCharacterMovement()->bOrientRotationToMovement = false;


	//이 회전 레이트로.
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = .2f;


	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (CameraComp)
	{
		CamCurrentFOV = CamDefaultFOV = CameraComp->FieldOfView;
	}

	EquipWeapon(SpawnDefaultWeapon());
	InitializeAmmoMap();
}

float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CalculateFOV(DeltaTime);

	CalculateAimingSensivity();

	CalculateCrosshairSpread(DeltaTime);

	//오버랩된 아이템 카운트를 통해 충돌 체크한다.
	TraceForItems();

}

void AShooterCharacter::CalculateFOV(float DeltaTime)
{
	float CamDestFOV = bAiming ? CamZoomedFOV : CamDefaultFOV;
	CamCurrentFOV = FMath::FInterpTo(CamCurrentFOV, CamDestFOV, DeltaTime, ZoomInterpSpeed);
	CameraComp->SetFieldOfView(CamCurrentFOV);
}

void AShooterCharacter::CalculateAimingSensivity()
{
	BaseTurnRate = bAiming ? AimingTurnRate : HipTurnRate;
	BaseLookUpRate = bAiming ? AimingLookUpRate : HipLookUpRate;
}

void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{ 0.f, 600.f };
	FVector2D VelocityMultiplierRange{ 0.f, 1.f };
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0.f;


	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

	float DestInAirFactor{ (GetCharacterMovement()->IsFalling()) ? 2.25f : 0.f };
	float InAirInterpSpeed{ (GetCharacterMovement()->IsFalling()) ? 2.25f : 30.f };
	CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, DestInAirFactor, DeltaTime, InAirInterpSpeed);

	float DestAimingFactor{ (bAiming) ? 0.6f : 0.f };
	float AimingInterpSpeed{ 30.f };
	CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, DestAimingFactor, DeltaTime, AimingInterpSpeed);

	float DestShootingFactor{ (bFiringBullet) ? 0.3f : 0.f };
	float ShootingInterpSpeed{ 60.f };
	CrosshairShootFactor = FMath::FInterpTo(CrosshairShootFactor, DestShootingFactor, DeltaTime, ShootingInterpSpeed);

	CrosshairSpreadMultiplier = .5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootFactor;
}

void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AShooterCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AShooterCharacter::MoveRight);

	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AShooterCharacter::TurnByMouse);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AShooterCharacter::LookUpByMouse);

	PlayerInputComponent->BindAxis(TEXT("TurnRate"), this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis(TEXT("LookUpRate"), this, &AShooterCharacter::LookUpAtRate);


	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction(TEXT("FireButton"), IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction(TEXT("FireButton"), IE_Released, this, &AShooterCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction(TEXT("AimingButton"), IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction(TEXT("AimingButton"), IE_Released, this, &AShooterCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction(TEXT("SelectButton"), IE_Pressed, this, &AShooterCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction(TEXT("SelectButton"), IE_Released, this, &AShooterCharacter::SelectButtonReleased);

	PlayerInputComponent->BindAction(TEXT("ReloadButton"), IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction(TEXT("CrouchButton"), IE_Pressed, this, &AShooterCharacter::CrouchButtonPressed);

}

void AShooterCharacter::FireWeapon()
{
	// 착용중인 무기가 nullptr일 경우
	if (EquippedWeapon == nullptr) return;

	// 평범한 상황이 아니라면 return
	if (CombatState != ECombatState::ECS_Unoccupied) return;

	if (WeaponHasAmmo())
	{
		// 사운드 재생
		PlayFireSound();

		// 에임 벌어짐
		StartCrosshairBulletFire();

		// 총알쏘기
		SendBullet();

		// 애니메이션 몽타주
		PlayGunfireMontage();

		// 총알 감소
		EquippedWeapon->DecrementAmmo();

		StartFireTimer();
	}
}

bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLoc, FVector& OutBeamLoc)
{
	FHitResult CrosshairHitResult;

	//여기서 기본 beam location 업데이트
	bool bCrosshairHit{ TraceUnderCrosshairs(CrosshairHitResult, OutBeamLoc) };

	FHitResult WeaponTraceHit;
	const FVector WeaponTraceStart{ MuzzleSocketLoc };
	const FVector WeaponTraceEnd{ OutBeamLoc + (OutBeamLoc - MuzzleSocketLoc) };

	GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd, ECollisionChannel::ECC_Visibility);

	if (WeaponTraceHit.bBlockingHit)
	{
		OutBeamLoc = WeaponTraceHit.Location;
	}


	return WeaponTraceHit.bBlockingHit;
}

void AShooterCharacter::AimingButtonPressed()
{
	bAiming = true;
}

void AShooterCharacter::AimingButtonReleased()
{
	bAiming = false;
}

//발사 트리거
void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();
}

void AShooterCharacter::StartFireTimer()
{
	CombatState = ECombatState::ECS_FireTimerInProgress;
	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::ResetAutoFire, AutomaticFireRate);
}

void AShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AShooterCharacter::SelectButtonPressed()
{
	if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this);

		if (TraceHitItem->GetPickupSound())
		{
			UGameplayStatics::PlaySound2D(this, TraceHitItem->GetPickupSound());
		}
	}
}

void AShooterCharacter::SelectButtonReleased()
{
}

void AShooterCharacter::CrouchButtonPressed()
{
	if (!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}
}

void AShooterCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	DropWeapon();
	EquipWeapon(WeaponToSwap);

	TraceHitItem = TraceHitItemLastFrame = nullptr;
}

void AShooterCharacter::ResetAutoFire()
{
	CombatState = ECombatState::ECS_Unoccupied;

	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed)
			FireWeapon();
	}
	else
	{
		// Reload Weapon
		ReloadWeapon();
	}
}

bool AShooterCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLoc)
{
	FVector2D ViewportSize{ CustomUtils::GetViewportSize() };

	FVector2D CrosshairProjLocation{ ViewportSize / 2.f };
	CrosshairProjLocation.Y -= 50.f;

	FVector CrosshairWorldPos, CrosshairWorldDir;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairProjLocation, CrosshairWorldPos, CrosshairWorldDir);

	if (bScreenToWorld)
	{
		const FVector Start{ CrosshairWorldPos };
		const FVector End{ Start + CrosshairWorldDir * ShotDistance };
		OutHitLoc = End;

		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECollisionChannel::ECC_Visibility);

		if (OutHitResult.bBlockingHit)
		{
			OutHitLoc = OutHitResult.Location;
			return true;
		}
	}

	return false;
}

void AShooterCharacter::TraceForItems()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);

		//아이템이 시야에 들어왔다.
		if (ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());

			if (TraceHitItem)
			{
				TraceHitItem->ShowWidget();

				if (TraceHitItemLastFrame && TraceHitItemLastFrame != TraceHitItem)
				{
					TraceHitItemLastFrame->HideWidget();
				}

				TraceHitItemLastFrame = TraceHitItem;
			}
		}
	}
	else if (TraceHitItemLastFrame)
	{
		TraceHitItemLastFrame->HideWidget();
	}
}

AWeapon* AShooterCharacter::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}

	return nullptr;
}

void AShooterCharacter::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip)
	{
		const USkeletalMeshSocket* HandSocket{ GetMesh()->GetSocketByName("RightHandSocket") };
		WeaponToEquip->HideWidget();
		if (HandSocket)
		{
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}

		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AShooterCharacter::DropWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->GetItemMesh()->DetachFromComponent({ EDetachmentRule::KeepWorld, true });
		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

void AShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);

}

bool AShooterCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr) return false;

	return EquippedWeapon->GetAmmo() > 0;
}

FVector2D AShooterCharacter::MakeRaisedCrosshairLocation() const
{
	FVector2D ViewportSize;

	if (GEngine && GEngine->GameViewport)
		GEngine->GameViewport->GetViewportSize(ViewportSize);

	ViewportSize /= 2.f;
	ViewportSize.Y -= 50.f;

	return ViewportSize;
}

void AShooterCharacter::PlayFireSound() const
{
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}
}

void AShooterCharacter::SendBullet()
{
	const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");
	if (BarrelSocket)
	{
		// 총구 화염
		SpawnMuzzleFX();

		// 이펙트, 트레일
		const FTransform SocketTransform{ BarrelSocket->GetSocketTransform(GetMesh()) };
		FVector BeamEnd;
		if (GetBeamEndLocation(SocketTransform.GetLocation(), BeamEnd))
			SpawnImpactFX(BeamEnd, SocketTransform);
	}
}

void AShooterCharacter::PlayGunfireMontage()
{
	// Hip Fire 애니메이션 몽타주
	UAnimInstance* AnimInstance{ GetMesh()->GetAnimInstance() };
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AShooterCharacter::SpawnMuzzleFX() const
{
	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");

	if (BarrelSocket && MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, BarrelSocket->GetSocketTransform(GetMesh()));
	}
}

void AShooterCharacter::SpawnImpactFX(const FVector& SpawnLocation, const FTransform& Transform) const
{
	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, SpawnLocation);
	}

	if (BeamParticles)
	{
		UParticleSystemComponent* Beam{ UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles,Transform) };
		Beam->SetVectorParameter("Target", SpawnLocation);
	}
}

void AShooterCharacter::ReloadButtonPressed()
{

	// 애니메이션만 작동한다.
	ReloadWeapon();

}

void AShooterCharacter::ReloadWeapon()
{
	// 평범한 상태가 아닐경우
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (EquippedWeapon == nullptr) return;

	if (CarryingAmmo() && !EquippedWeapon->IsClipFull())
	{
		CombatState = ECombatState::ECS_Reloading;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
		}
	}
}

// 애니메이션 후, 몽타주가 종료되면 알아서 호출
void AShooterCharacter::FinishReloading()
{
	CombatState = ECombatState::ECS_Unoccupied;

	if (EquippedWeapon == nullptr) return;

	// 탄 종류는 무엇인가?
	const auto AmmoType{ EquippedWeapon->GetAmmoType() };

	if (AmmoMap.Contains(AmmoType))
	{
		// 종류에 맞는 현재 가지고 있는 탄약
		int32 CarriedAmmo = AmmoMap[AmmoType];

		// 탄창의 빈 공간
		const int32 MagEmptySpace = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

		// 가지고 있는 탄약보다, 비어있는 공간이 더 클 경우. 가지고 있는 탄약만 충전해야한다.
		if (MagEmptySpace > CarriedAmmo)
		{
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
		// 일반적인 상황일 경우, 비어있는 탄약만큼 충전하고, 충전한만큼 감소시킨다.
		else
		{
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}

}

bool AShooterCharacter::CarryingAmmo()
{
	if (EquippedWeapon == nullptr) return false;

	auto AmmoType = EquippedWeapon->GetAmmoType();

	if (AmmoMap.Contains(AmmoType))
	{
		return AmmoMap[AmmoType] > 0;
	}

	return false;
}

void AShooterCharacter::GrabClip()
{
	if (EquippedWeapon == nullptr) return;
	if (HandSceneComponent == nullptr) return;


	// 탄창 뼈의 인덱스를 받아왔다.
	int32 ClipBoneIndex{ EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName()) };

	// 탄창의 트랜스폼 받음
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	// HandSceneComponent를 Hand_L위치에 붙인다.
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("Hand_L")));
	// HandSceneComponent를 탄창위치로 변경
	HandSceneComponent->SetWorldTransform(ClipTransform);

	// 이후 Hand_L에 귀속되어 HandSceneComponent가 움직이게되는데, 탄창의 움직임은 BP에서 다룬다!

	EquippedWeapon->SetMovingClip(true);
}

void AShooterCharacter::ReleaseClip()
{
	EquippedWeapon->SetMovingClip(false);
}

void AShooterCharacter::IncrementOverlappedItemCount(int8 Amount)
{

	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}

}

FVector AShooterCharacter::GetCameraInterpLocation()
{
	const FVector CameraWorldLocation = { CameraComp->GetComponentLocation() };
	const FVector CameraForward = { CameraComp->GetForwardVector() };
	return CameraWorldLocation + CameraForward * CameraInterpDistance + FVector(0.f, 0.f, CameraInterpElevation);
}

void AShooterCharacter::GetPickupItem(AItem* Item)
{
	if (Item->GetEquipSound())
	{
		UGameplayStatics::PlaySound2D(this, Item->GetEquipSound());
	}
	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon)
	{
		SwapWeapon(Weapon);
	}

}

void AShooterCharacter::MoveForward(float AxisValue)
{
	if (AxisValue == 0.f)
		return;

	const FRotator YawRotation{ 0.f, GetControlRotation().Yaw, 0 };

	const FVector DirectionVec{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
	AddMovementInput(DirectionVec, AxisValue * MoveSpeed * GetWorld()->DeltaTimeSeconds);
}

void AShooterCharacter::MoveRight(float AxisValue)
{
	if (AxisValue == 0.f)
		return;

	const FRotator Rotation{ GetControlRotation() };
	const FRotator YawRotation{ 0.f, GetControlRotation().Yaw, 0 };

	const FVector DirectionVec{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
	AddMovementInput(DirectionVec, AxisValue * MoveSpeed * GetWorld()->DeltaTimeSeconds);
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::TurnByMouse(float Rate)
{
	if (bAiming)
		Rate *= AimingMouseSensibity;

	AddControllerYawInput(Rate);

}

void AShooterCharacter::LookUpByMouse(float Rate)
{
	if (bAiming)
		Rate *= AimingMouseSensibity;

	AddControllerPitchInput(Rate);

}



