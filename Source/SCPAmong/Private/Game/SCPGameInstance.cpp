// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SCPGameInstance.h"
#include "Game/SCPOnlineSession.h"
#include "Online.h"
#include "OnlineSubsystemUtils.h"

TSubclassOf<UOnlineSession> USCPGameInstance::GetOnlineSessionClass()
{
	return USCPOnlineSession::StaticClass();
}

void USCPGameInstance::Init()
{
	Super::Init();

	UpdateRichPresence(TEXT("AtMainMenu"), TEXT(""), false, false);
}

void USCPGameInstance::UpdateRichPresence(FString Status, FString Score, bool bUseScore, bool bPlaying)
{

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get("Steam");

	if (!OnlineSub)
		return;

	IOnlineIdentityPtr IdentityInterface = OnlineSub->GetIdentityInterface();
	IOnlinePresencePtr PresenceInterface = OnlineSub->GetPresenceInterface();

	if (!IdentityInterface.IsValid())
		return;

	if (!PresenceInterface.IsValid())
		return;

	TSharedPtr<FOnlineUserPresence> PresenceData;
	TSharedPtr<const FUniqueNetId> CurrentUser = IdentityInterface->GetUniquePlayerId(0);

	if (!CurrentUser.IsValid())
		return;

	EOnlineCachedResult::Type Result = PresenceInterface->GetCachedPresence(*CurrentUser, PresenceData);

	if (Result == EOnlineCachedResult::Type::NotFound)
		PresenceData = MakeShared<FOnlineUserPresence>(FOnlineUserPresence());

	if (PresenceData.IsValid())
	{
		PresenceData->Status.StatusStr = bPlaying ? "Playing" : "Idle";
		PresenceData->Status.Properties.FindOrAdd("steam_display", FVariantData(bUseScore ? FString("#StatusWithScore") : FString("#StatusWithoutScore")));
		PresenceData->Status.Properties.FindOrAdd("gamestatus", FVariantData(Status));
		PresenceData->Status.Properties.FindOrAdd("score", FVariantData(Score));
		PresenceInterface->SetPresence(*CurrentUser, PresenceData->Status, IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateUObject(this, &USCPGameInstance::OnPresencePushComplete));
	}
}

void USCPGameInstance::OnPresencePushComplete(const class FUniqueNetId& UniqueId, const bool bSuccess)
{

}