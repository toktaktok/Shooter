// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ServerRow.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API UServerRow : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(class UMainMenu* InParent, uint32 InIndex);

	UPROPERTY(BlueprintReadOnly)
	bool Selected{ false };

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ServerName;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Host;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ConnectionState;

	UPROPERTY(meta = (BindWidget))
	class UButton* RowButton;

	UPROPERTY()
	class UMainMenu* Parent;

private:
	UFUNCTION()
	void OnClicked();


	uint32 Index;

};
