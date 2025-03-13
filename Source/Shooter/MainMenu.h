// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MenuWidget.h"

#include "MainMenu.generated.h"


class UWidgetSwitcher;
class UEditableTextBox;
class UWidget;
class UButton;
class UUserWidget;

USTRUCT()
struct FServerData
{
	GENERATED_BODY()

	FString Name;
	uint16 CurrentPlayers;
	uint16 MaxPlayers;
	FString HostUserName;
};

UCLASS()
class SHOOTER_API UMainMenu : public UMenuWidget
{
	GENERATED_BODY()

public:
	UMainMenu(const FObjectInitializer& ObjectInitializer);

	/*setters*/
	void SetMenuInterface(IMenuInterface* NewMenuInterface);

	void UpdateServerList(TArray<FServerData> ServerInfos);

	void SelectIndex(uint32 Index);

protected:
	virtual bool Initialize();

private:

	//������ Host Menu�� ����ġ.
	UFUNCTION()
	void OpenHostMenu();

	UFUNCTION()
	void HostSession();

	//Join�޴��� ����ġ�ϸ鼭 ���� ����Ʈ ������ ��û.
	UFUNCTION()
	void OpenJoinMenu();

	UFUNCTION()
	void JoinSession();

	UFUNCTION()
	void OpenTitle();

	void UpdateSelectedState();

	TSubclassOf<UUserWidget> ServerRowClass;

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* MenuSwitcher;

	UPROPERTY(meta = (BindWidget))
	UWidget* Title;

	UPROPERTY(meta = (BindWidget))
	UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;
	//ȣ��Ʈ �޴�
	UPROPERTY(meta = (BindWidget))
	UWidget* HostMenu;

	UPROPERTY(meta = (BindWidget))
	UEditableTextBox* SessionNameBox;

	UPROPERTY(meta = (BindWidget))
	UButton* HostCreateButton;

	UPROPERTY(meta = (BindWidget))
	UButton* HostCancelButton;

	//���� �޴�
	UPROPERTY(meta = (BindWidget))
	UWidget* JoinMenu;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinJoinButton;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinCancelButton;

	UPROPERTY(meta = (BindWidget))
	class UPanelWidget* ServerList;

	TOptional<uint32> SelectedIndex;

};
