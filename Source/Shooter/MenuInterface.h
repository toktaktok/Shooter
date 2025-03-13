// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MenuInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMenuInterface : public UInterface
{
	GENERATED_BODY()
};

class SHOOTER_API IMenuInterface
{
	GENERATED_BODY()

public:
	virtual void Host(FString ServerName) = 0;
	virtual void Join(uint32 Idx) = 0;
	virtual void LoadMainMenu() = 0;
	virtual void RefreshServerList() = 0;
};
