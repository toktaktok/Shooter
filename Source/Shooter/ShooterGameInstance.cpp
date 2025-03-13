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

	//현재 OSS 환경
	UE_LOG(LogTemp, Warning, TEXT("Current OSS: %s"), *OSS->GetSubsystemName().ToString());

	//세션 인터페이스 대입
	SessionInterface = OSS->GetSessionInterface();

	if (SessionInterface.IsValid())
	{
		// 세션 생성 완료 또는 삭제 완료, 찾기 완료 시 콜백 함수를 바인딩한다.
		SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UShooterGameInstance::OnCreateSessionComplete);
		SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UShooterGameInstance::OnFindSessionComplete);
		SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UShooterGameInstance::OnJoinSessionComplete);
		SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UShooterGameInstance::OnDestroySessionComplete);
	}

	//연결 해제되었을 때의 델리게이트.
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

	// 이 세션은 로컬 네트워크 연결이니? NULL 시스템일 때 true가 된다.
	SessionSet.bIsLANMatch = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL");

	// 플레이어의 온라인 상태, 친구 목록 등 게임 세션에 쉽게 참여할 수 있도록 하는 프레즌스 기능
	SessionSet.bUsesPresence = true;
	SessionSet.Set(SERVER_SET_KEY, DesiredServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 최대 N명의 플레이어가 접속할 수 있다.
	SessionSet.NumPublicConnections = DesiredNumPlayer;

	// 세션을 네트워크 상에 노출시킬지? 다른 플레이어들이 이 세션을 검색하여 참여할 수 있다.
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
		//NULL 서브시스템일 때만 lan query
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
			//세션 이름
			Data.Name = SearchResult.GetSessionIdStr();
			//접속 상태
			Data.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
			Data.CurrentPlayers = Data.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
			//호스트 이름
			int32 MaxChar{ 8 };
			Data.HostUserName = SearchResult.Session.OwningUserName.Left(MaxChar);

			//세션 정보를 확인한다.
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

	//진입 메시지...
	GEngine->AddOnScreenDebugMessage(0, 5, FColor::Green, FString::Printf(TEXT("Joining %s"), *Address));

	//첫번째 로컬 플레이어 컨트롤러를 가져와서, 트레벨
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


