// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

//OSS
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "MenuInterface.h"
#include "ShooterGameInstance.generated.h"

/**
 * 
 */

class FOnlineSessionSearch;
class UUserWidget;
class UMainMenu;

UCLASS()
class SHOOTER_API UShooterGameInstance : public UGameInstance, public IMenuInterface
{
	GENERATED_BODY()

public:
	UShooterGameInstance(const FObjectInitializer& ObjectInitializer);

	virtual void Init();

	//서버를 호스팅한다.
	UFUNCTION(Exec)
	void Host(FString ServerName) override;

	UFUNCTION(Exec)
	void Join(uint32 Index) override;

	//메인 메뉴의 위젯을 로드한다.
	UFUNCTION(BlueprintCallable)
	void LoadMainMenuWidget();

	void LoadMainMenu() override;
	void RefreshServerList() override;

protected:

private:
	//설정을 토대로, 실제 세션을 만든다.
	void CreateSession();

	//Delegates
	void OnCreateSessionComplete(FName SessionName, bool Success);
	void OnFindSessionComplete(bool Success);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool Success);
	void OnNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	//메뉴 위젯
	TSubclassOf<UUserWidget> MenuClass;
	UMainMenu* MainMenuWidget;

	//서버 정보
	FString DesiredServerName{};
	int32 DesiredNumPlayer{ 5 };

	//세션 관리
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

};
