// Fill out your copyright notice in the Description page of Project Settings.


#include "MenuWidget.h"

void UMenuWidget::SetUp()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	if (!ensure(GetWorld())) return;

	APlayerController* PlayerController{ GetWorld()->GetFirstPlayerController() };
	if (!ensure(PlayerController)) return;

	FInputModeUIOnly InputModeData;
	InputModeData.SetWidgetToFocus(TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(true);
}

void UMenuWidget::TearDown()
{
	this->RemoveFromViewport();

	if (!ensure(GetWorld())) return;

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!ensure(PlayerController)) return;

	FInputModeGameOnly InputModeData;
	PlayerController->SetInputMode(InputModeData);
	PlayerController->SetShowMouseCursor(false);
}

void UMenuWidget::SetMenuInterface(IMenuInterface* NewInterface)
{
	MenuInterface = NewInterface;
}
