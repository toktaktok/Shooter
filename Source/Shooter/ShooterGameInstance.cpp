#include "ShooterGameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "OnlineSessionSettings.h"

#include "MainMenu.h"

const static FName SERVER_SET_KEY{ TEXT("ServerName") };

UShooterGameInstance::UShooterGameInstance(const FObjectInitializer& ObjectInitializer)
{
	ConstructorHelpers::FClassFinder<UUserWidget> MenuBPClass(TEXT("/Game/_Game/UI/WBP_MainMenu"));
	if (!ensure(MenuBPClass.Class)) return;

	MenuClass = MenuBPClass.Class;
}


void UShooterGameInstance::Init()
{
	Super::Init();

	IOnlineSubsystem* OSS{ IOnlineSubsystem::Get() };
	if (!OSS)
	{
		UE_LOG(LogTemp, Warning, TEXT("No Subsystem."));
		return;
	}

	//���� OSS ȯ��
	UE_LOG(LogTemp, Warning, TEXT("Current OSS: %s"), *OSS->GetSubsystemName().ToString());

	//���� �������̽� ����
	SessionInterface = OSS->GetSessionInterface();

	if (SessionInterface.IsValid())
	{
		// ���� ���� �Ϸ� �Ǵ� ���� �Ϸ�, ã�� �Ϸ� �� �ݹ� �Լ��� ���ε��Ѵ�.
		SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UShooterGameInstance::OnCreateSessionComplete);
		SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UShooterGameInstance::OnFindSessionComplete);
		SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UShooterGameInstance::OnJoinSessionComplete);
		SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UShooterGameInstance::OnDestroySessionComplete);
	}

	//���� �����Ǿ��� ���� ��������Ʈ.
	if (GEngine)
		GEngine->OnNetworkFailure().AddUObject(this, &UShooterGameInstance::OnNetworkFailure);
}

void UShooterGameInstance::LoadMainMenu()
{
	APlayerController* PlayerController{ GetFirstLocalPlayerController() };
	if (!ensure(PlayerController)) return;

	PlayerController->ClientTravel("/Game/_Game/Maps/MainMenu", ETravelType::TRAVEL_Absolute);
}

void UShooterGameInstance::LoadMainMenuWidget()
{
	if (!ensure(MenuClass)) return;
	MainMenuWidget = CreateWidget<UMainMenu>(this, MenuClass);

	if (!ensure(MainMenuWidget)) return;
	MainMenuWidget->SetMenuInterface(this);
	MainMenuWidget->SetUp();
}

void UShooterGameInstance::Host(FString ServerName)
{
	if (!SessionInterface.IsValid()) return;

	DesiredServerName = ServerName;
	auto ExistingSession{ SessionInterface->GetNamedSession(NAME_GameSession) };

	if (ExistingSession != nullptr)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}
	else
	{
		CreateSession();
	}

}

void UShooterGameInstance::CreateSession()
{
	if (!SessionInterface.IsValid()) return;

	FOnlineSessionSettings SessionSet;

	// �� ������ ���� ��Ʈ��ũ �����̴�? NULL �ý����� �� true�� �ȴ�.
	SessionSet.bIsLANMatch = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL");

	// �÷��̾��� �¶��� ����, ģ�� ��� �� ���� ���ǿ� ���� ������ �� �ֵ��� �ϴ� ������ ���
	SessionSet.bUsesPresence = true;
	SessionSet.Set(SERVER_SET_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// �ִ� N���� �÷��̾ ������ �� �ִ�.
	SessionSet.NumPublicConnections = DesiredNumPlayer;

	// ������ ��Ʈ��ũ �� �����ų��? �ٸ� �÷��̾���� �� ������ �˻��Ͽ� ������ �� �ִ�.
	SessionSet.bShouldAdvertise = true;
	SessionSet.bUseLobbiesIfAvailable = true;

	//SessionSet.bAllowJoinInProgress = true;
	//SessionSet.bAllowJoinViaPresence = true;

	SessionInterface->CreateSession(0, NAME_GameSession, SessionSet);
}

void UShooterGameInstance::OnCreateSessionComplete(FName SessionName, bool Success)
{
	if (!Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not create Session"));
		return;
	}

	if (MainMenuWidget)
		MainMenuWidget->TearDown();

	if(GEngine)
		GEngine->AddOnScreenDebugMessage(0, 2, FColor::Green, TEXT("Hosting..."));

	UWorld* World = GetWorld();
	if (!ensure(World != nullptr)) return;

	World->ServerTravel("/Game/_Game/Maps/Lobby?listen");
}

void UShooterGameInstance::RefreshServerList()
{
	SessionSearch = MakeShareable(new FOnlineSessionSearch);
	if (SessionSearch.IsValid())
	{
		//NULL ����ý����� ���� lan query
		SessionSearch->bIsLanQuery = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL");
		SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
		SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());

		UE_LOG(LogTemp, Warning, TEXT("Finding Session Start"));
	}
}
void UShooterGameInstance::OnFindSessionComplete(bool Success)
{
	if (Success && SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Finding Session Finished"));

		TArray<FServerData> ServerInfos;
		for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
		{
			UE_LOG(LogTemp, Warning, TEXT("Found Session Names: %s"), *SearchResult.GetSessionIdStr());

			FServerData Data;
			//���� �̸�
			Data.Name = SearchResult.GetSessionIdStr();
			//���� ����
			Data.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
			Data.CurrentPlayers = Data.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
			//ȣ��Ʈ �̸�
			int32 MaxChar{ 8 };
			Data.HostUserName = SearchResult.Session.OwningUserName.Left(MaxChar);

			//���� ������ Ȯ���Ѵ�.
			FString ServerName;
			if (SearchResult.Session.SessionSettings.Get(SERVER_SET_KEY, ServerName))
			{
				Data.Name = ServerName;
			}
			else
			{
				Data.Name = "Could not find name.";
			}

			ServerInfos.Add(Data);
		}

		MainMenuWidget->UpdateServerList(ServerInfos);
	}
}

void UShooterGameInstance::Join(uint32 Index)
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid()) return;

	if (MainMenuWidget)
		MainMenuWidget->TearDown();

	SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[Index]);
}


void UShooterGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!SessionInterface.IsValid() || Result == EOnJoinSessionCompleteResult::SessionDoesNotExist) return;

	FString Address{};

	if (!SessionInterface->GetResolvedConnectString(SessionName, Address))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not get ConnectString!!"));
		return;
	}

	if (!ensure(GEngine)) return;

	//���� �޽���...
	GEngine->AddOnScreenDebugMessage(0, 5, FColor::Green, FString::Printf(TEXT("Joining %s"), *Address));

	//ù��° ���� �÷��̾� ��Ʈ�ѷ��� �����ͼ�, Ʈ����
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!ensure(PlayerController)) return;

	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);

}

void UShooterGameInstance::OnDestroySessionComplete(FName SessionName, bool Success)
{
	
}

void UShooterGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	LoadMainMenu();
}


