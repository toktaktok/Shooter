// Fill out your copyright notice in the Description page of Project Settings
#include "LobbyModeBase.h"

ALobbyModeBase::ALobbyModeBase()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/_Game/Character/Shooter/BP_ShooterCharacter"));
	
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
