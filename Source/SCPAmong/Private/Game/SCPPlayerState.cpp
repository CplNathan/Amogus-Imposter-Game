// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SCPPlayerState.h"
#include "Game/SCPGameState.h"
#include "Game/SCPCharacterBase.h"
#include "Net/UnrealNetwork.h"
#include "Online.h"
//#include "OnlineSubsystem.h"

ASCPPlayerState::ASCPPlayerState()
{
	bPlaying = false;
	bIsImposter = false;
	bDead = false;
	WorldKillTime = -1;
}

void ASCPPlayerState::BeginPlay()
{
	IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface();
	
	if (VoiceInterface.IsValid())
	{
		VoiceInterface->RegisterLocalTalker(0);
	}
}

void ASCPPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCPPlayerState, ChatConfig);

	DOREPLIFETIME(ASCPPlayerState, bPlaying);
	DOREPLIFETIME(ASCPPlayerState, Colour);

	DOREPLIFETIME(ASCPPlayerState, MeetingData);

	DOREPLIFETIME_CONDITION(ASCPPlayerState, bIsImposter, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPPlayerState, Imposters, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPPlayerState, Kills, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ASCPPlayerState, WorldKillTime, COND_OwnerOnly);

	DOREPLIFETIME(ASCPPlayerState, bDead);
}

void ASCPPlayerState::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	ASCPGameState* GameState = GetWorld()->GetGameState<ASCPGameState>();
	DOREPLIFETIME_ACTIVE_OVERRIDE(ASCPPlayerState, bDead, GameState->IsVotingTime()); // Only replicates to clients during voting
}

void ASCPPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ASCPPlayerState* CustomState = Cast<ASCPPlayerState>(PlayerState);
	CustomState->bPlaying = bPlaying;
	CustomState->Imposters = Imposters;
	CustomState->bIsImposter = bIsImposter;
	CustomState->Kills = Kills;
	CustomState->bDead = bDead;
}

void ASCPPlayerState::OverrideWith(APlayerState* PlayerState)
{
	Super::OverrideWith(PlayerState);

	ASCPPlayerState* CustomState = Cast<ASCPPlayerState>(PlayerState);
	bPlaying = CustomState->bPlaying;
	Imposters = CustomState->Imposters;
	bIsImposter = CustomState->bIsImposter;
	Kills = CustomState->Kills;
	bDead = CustomState->bDead;
}

bool ASCPPlayerState::CanSendChatMessage(const FString& Message, float RealTimeSeconds, bool bIgnoreCooldown)
{
	if (!GetWorld()->GetGameState<ASCPGameState>()->IsVotingTime())
		return false;

	if (Message.Len() > ChatConfig.CharacterLimit)
		return false;

	if ((RealTimeSeconds - ChatConfig.LastMessageSent <= ChatConfig.TimeBetweenMessages) && !bIgnoreCooldown)
		return false;

	if (ChatConfig.bMuted)
		return false;

	if (bDead)
		return false;

	return true;
}

bool ASCPPlayerState::CanSubmitVote(ASCPPlayerState* Victim, bool bSkip)
{
	if (!GetWorld()->GetGameState<ASCPGameState>()->IsVotingTime())
		return false;

	if (MeetingData.bVoted || MeetingData.bSkipped)
		return false;

	if (bDead)
		return false;

	if (Victim && !bSkip)
	{
		if (Victim == this)
			return false;

		if (Victim->bDead)
			return false;
	}

	return true;
}

void ASCPPlayerState::ReplPawnData()
{
	// Should only be called by gamemode
	ASCPCharacterBase* Character = GetPawn<ASCPCharacterBase>();
	if (Character)
	{
		Character->bIsImposter = bIsImposter;
		Character->Colour = Colour;
		Character->bDead = bDead;

		Character->OnVisualUpdate();
	}
}

void ASCPPlayerState::UpdateMeetingData(FMeetingData OldData)
{
	if (OldData.VotedBy != MeetingData.VotedBy)
		RevealVoted.Broadcast(MeetingData.VotedBy);
}

void ASCPPlayerState::UpdateImposters()
{
	for (APlayerState* PlayerState : Imposters)
	{
		ASCPPlayerState* SCPPlayerState = Cast<ASCPPlayerState>(PlayerState);
		SCPPlayerState->bIsImposter = true;
	}
}

void ASCPPlayerState::UpdateImposterKills()
{
	for (APlayerState* PlayerState : Kills)
	{
		ASCPPlayerState* SCPPlayerState = Cast<ASCPPlayerState>(PlayerState);
		SCPPlayerState->bDead = true;
	}
}