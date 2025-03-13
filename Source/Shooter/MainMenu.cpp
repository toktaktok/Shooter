// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenu.h"
#include "UObject/ConstructorHelpers.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "ServerRow.h"


UMainMenu::UMainMenu(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UUserWidget> ServerRowBPClass(TEXT("/Game/_Game/UI/WBP_ServerRow"));
	if (!ensure(ServerRowBPClass.Class != nullptr)) return;

	ServerRowClass = ServerRowBPClass.Class;
}

bool UMainMenu::Initialize()
{
	if (!Super::Initialize()) return false;

	if (!ensure(HostButton)) return false;
	HostButton->OnClicked.AddDynamic(this, &UMainMenu::OpenHostMenu);

	if (!ensure(HostCreateButton)) return false;
	HostCreateButton->OnClicked.AddDynamic(this, &UMainMenu::HostSession);

	if (!ensure(HostCancelButton)) return false;
	HostCancelButton->OnClicked.AddDynamic(this, &UMainMenu::OpenTitle);

	if (!ensure(JoinButton)) return false;
	JoinButton->OnClicked.AddDynamic(this, &UMainMenu::OpenJoinMenu);

	if (!ensure(JoinJoinButton)) return false;
	JoinJoinButton->OnClicked.AddDynamic(this, &UMainMenu::JoinSession);

	if (!ensure(JoinCancelButton)) return false;
	JoinCancelButton->OnClicked.AddDynamic(this, &UMainMenu::OpenTitle);

	return true;
}

void UMainMenu::SetMenuInterface(IMenuInterface* NewMenuInterface)
{
	MenuInterface = NewMenuInterface;
}

void UMainMenu::UpdateServerList(TArray<FServerData> ServerInfos)
{
	UWorld* World = this->GetWorld();
	if (!ensure(World != nullptr)) return;

	ServerList->ClearChildren();

	uint32 i = 0;

	// 서버 이름들을 싹다 가져와서 ServerRow에 출력한다.
	for (const FServerData& ServerInfo : ServerInfos)
	{
		UServerRow* Row = CreateWidget<UServerRow>(World, ServerRowClass);
		if (!ensure(Row != nullptr)) return;

		Row->ServerName->SetText(FText::FromString(ServerInfo.Name));
		Row->Host->SetText(FText::FromString(ServerInfo.HostUserName));

		FString Capacity{ FString::Printf(TEXT("%d / %d"),ServerInfo.CurrentPlayers, ServerInfo.MaxPlayers) };
		Row->ConnectionState->SetText(FText::FromString(Capacity));

		Row->Setup(this, i);
		++i;

		ServerList->AddChild(Row);
	}
}

void UMainMenu::SelectIndex(uint32 Index)
{
	UE_LOG(LogTemp, Display, TEXT("Index %d Selected"), Index);
	SelectedIndex = Index;
	UpdateSelectedState();
}

void UMainMenu::UpdateSelectedState()
{
	for (int32 i{}; i < ServerList->GetChildrenCount(); ++i)
	{
		auto Row{ Cast<UServerRow>(ServerList->GetChildAt(i)) };
		if (Row)
		{
			Row->Selected = (SelectedIndex.IsSet() && SelectedIndex.GetValue() == i);
		}
	}
}

void UMainMenu::OpenHostMenu()
{
	if (!ensure(MenuSwitcher) || !ensure(HostMenu)) return;

	MenuSwitcher->SetActiveWidget(HostMenu);
}

void UMainMenu::HostSession()
{
	if (MenuInterface != nullptr)
	{
		FString ServerName{ SessionNameBox->Text.ToString() };
		MenuInterface->Host(ServerName);
	}
}

void UMainMenu::OpenJoinMenu()
{
	if (!ensure(MenuSwitcher) || !ensure(JoinMenu)) return;

	MenuSwitcher->SetActiveWidget(JoinMenu);

	if (MenuInterface)
		MenuInterface->RefreshServerList();

}

void UMainMenu::JoinSession()
{
	if (!MenuInterface) return;

	if (SelectedIndex.IsSet())
	{
		UE_LOG(LogTemp, Warning, TEXT("Selected index %d"), SelectedIndex.GetValue());
		MenuInterface->Join(SelectedIndex.GetValue());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Selected index not Set"));
		return;
	}
}

void UMainMenu::OpenTitle()
{
	if (!ensure(MenuSwitcher) || !ensure(Title)) return;

	MenuSwitcher->SetActiveWidget(Title);
}



