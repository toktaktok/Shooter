// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UShooterAnimInstance::UShooterAnimInstance()
{
}

void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (!ShooterCharacter)
	{
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
		return;
	}

	UpdateSpeed();

	UpdateShooterState();

	FRotator AimRotation{ ShooterCharacter->GetBaseAimRotation() };
	FRotator MovementRotation{ UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity()) };

	MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

	if (ShooterCharacter->GetVelocity().Size() > 0.f)
		PreMovementOffsetYaw = MovementOffsetYaw;

	TurnInPlace();
	Lean(DeltaTime);
}

void UShooterAnimInstance::NativeInitializeAnimation()
{
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}

void UShooterAnimInstance::TurnInPlace()
{
	if (!ShooterCharacter) return;

	Pitch = ShooterCharacter->GetBaseAimRotation().Pitch;

	if (0 < Speed)
	{
		//회전할 필요가 없다! 캐릭터가 움직이기 때문
		RootYawOffset = 0.f;
		PreRotationCurve = RotationCurve = 0.f;
		TIPPreCharacterYaw = TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
	}
	else
	{
		TIPPreCharacterYaw = TIPCharacterYaw;
		TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;

		const float TIPYawDelta{ TIPCharacterYaw - TIPPreCharacterYaw };

		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TIPYawDelta);


		const float TurningValue{ GetCurveValue("Turning") };

		if (TurningValue > 0)
		{
			PreRotationCurve = RotationCurve;
			RotationCurve = GetCurveValue("Rotation");
			const float DeltaRotation{ RotationCurve - PreRotationCurve };

			//RootYawOffset이 양수라면 왼쪽, 음수라면 오른쪽
			(RootYawOffset > 0) ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;

			const float ABSRootYawOffset{ FMath::Abs(RootYawOffset) };
			if (ABSRootYawOffset > 90.f)
			{
				const float YawExcess{ ABSRootYawOffset - 90.f };
				RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
			}
		}

		/* 디버그 메시지
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, -1.f, FColor::Yellow, FString::Printf(TEXT("CharacterYaw: %f"), CharacterYaw));
			GEngine->AddOnScreenDebugMessage(2, -1.f, FColor::Red, FString::Printf(TEXT("PreCharacterYaw: %f"), PreCharacterYaw));
		}
		*/
	}
}

void UShooterAnimInstance::Lean(float DeltaTime)
{
	if (!ShooterCharacter) return;

	PreCharacterRot = CharacterRot;
	CharacterRot = ShooterCharacter->GetActorRotation();

	const FRotator RotDelta{ UKismetMathLibrary::NormalizedDeltaRotator(CharacterRot,PreCharacterRot) };

	const float Target{ static_cast<float>(RotDelta.Yaw) / DeltaTime };
	const float Interp{ FMath::FInterpTo(YawDelta,Target, DeltaTime, 6.f) };
	YawDelta = FMath::Clamp(Interp, -90.f, 90.f);
}

void UShooterAnimInstance::UpdateSpeed()
{
	FVector Velocity{ ShooterCharacter->GetVelocity() };
	Velocity.Z = 0.f;
	Speed = Velocity.Size();
}

void UShooterAnimInstance::UpdateShooterState()
{
	bReloading = (ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading);

	//Is The Character in the air?
	bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();
	//Is The Character Accelerating?
	bIsAccelerating = (ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f);

	bCrouching = ShooterCharacter->IsCrouching();

	bAiming = ShooterCharacter->IsAiming();
}
