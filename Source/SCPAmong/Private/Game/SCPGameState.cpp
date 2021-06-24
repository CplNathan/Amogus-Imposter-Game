// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SCPGameState.h"
#include "Game/SCPCharacterBase.h"
#include "Game/SCPPlayerState.h"
#include "Net/UnrealNetwork.h"

ASCPGameState::ASCPGameState()
{
	GameplayState = GameplayState::Playing;

	WorldReadyTime = -1;
	WorldVotingTime = -1;
}

void ASCPGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//DOREPLIFETIME_CONDITION(ASCPGameState, AllPlayers, COND_SimulatedOnly);

	DOREPLIFETIME(ASCPGameState, GameplayState);

	DOREPLIFETIME(ASCPGameState, InactivePlayerArray);

	DOREPLIFETIME(ASCPGameState, CurrentPlayers);
	DOREPLIFETIME(ASCPGameState, GameRules);
	DOREPLIFETIME(ASCPGameState, WorldReadyTime);
	DOREPLIFETIME(ASCPGameState, WorldVotingTime);
}

void ASCPGameState::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
    Super::PreReplication(ChangedPropertyTracker);
    //DOREPLIFETIME_ACTIVE_OVERRIDE(ASCPGameState, AllPlayers, HasMatchEnded()); // Only replicates to clients when game has ended
	/// BIG TODO: Look in AGameState, the AllPlayers array doesent need to exist... it contains the bImposter and bDead state but the server already knows this from the player state, instead of keeping this dumb master array instead just query all player states manually...
	/// // Because the PlayerState array may be removed when the player disconnects - we may need to overload the OnDisconnect/Logout function to only remove it if they are alive... Actually because this may cause issues when inactive player states return - there is probably an array for inactive players. we can just search both of them and do the various logic required...
}

TArray<ASCPPlayerState*> ASCPGameState::GetAlivePlayers()
{
	TArray<ASCPPlayerState*> SCPPlayerArray;
	for (APlayerState* PlayerState : PlayerArray)
	{
		SCPPlayerArray.Add((ASCPPlayerState*)PlayerState);
	}

	return SCPPlayerArray.FilterByPredicate([](ASCPPlayerState* v1) {
		return !v1->bDead && v1->bPlaying;
	});
}

TArray<ASCPPlayerState*> ASCPGameState::GetVotedPlayers()
{
	return GetAlivePlayers().FilterByPredicate([](ASCPPlayerState* v1) {
		return v1->MeetingData.bVoted;
	});
}

TArray<ASCPPlayerState*> ASCPGameState::GetNotVotedPlayers()
{
	return GetAlivePlayers().FilterByPredicate([](ASCPPlayerState* v1) {
		return !v1->MeetingData.bVoted || v1->MeetingData.bSkipped;
	});
}

bool ASCPGameState::CanKillPlayer(APlayerController* Caller, ASCPCharacterBase* TargetPawn, bool bConsiderCooldown)
{
	if (!Caller)
		return false;
	if (!TargetPawn)
		return false;

	if (MatchState != MatchState::InProgress || GameplayState != GameplayState::Playing)
		return false;

	ASCPPlayerState* CallingPlayer = Cast<ASCPPlayerState>(Caller->PlayerState);
	ASCPPlayerState* TargetPlayer = Cast<ASCPPlayerState>(TargetPawn->GetPlayerState());

	/*
	TArray<APlayerState*> AllPlayers = GetAllPlayers();

	if (GetNetMode() < NM_Client) // Server
	{
		// Find FPlayerData from authority for authoritive actions.
		CallingPlayer = AllPlayers.Find(Caller->PlayerState);
		CallingPlayer = AllPlayers.FindByPredicate([Caller](FPlayerData& InPlayer)
		{
			return InPlayer.PlayerState == Caller->PlayerState;
		});

		TargetPlayer = AllPlayers.FindByPredicate([TargetPawn](FPlayerData& InPlayer)
		{
			return InPlayer.PlayerState == TargetPawn->GetPlayerState();
		});
	}
	else
	{
		// Assemble FPlayerData based on what the client already knows.
		FPlayerData CallingPlayerObj = FPlayerData(Caller->GetPlayerState<ASCPPlayerState>());
		FPlayerData TargetPlayerObj = FPlayerData(TargetPawn->GetPlayerState<ASCPPlayerState>());
		CallingPlayer = &CallingPlayerObj;
		TargetPlayer = &TargetPlayerObj;
	}
	*/

	if (!CallingPlayer || !CallingPlayer->bIsImposter || CallingPlayer->bDead)
		return false;

	if (!TargetPlayer || TargetPlayer->bIsImposter || TargetPlayer->bDead)
		return false;


	if (!TargetPlayer->GetPawn()->IsNetRelevantFor(Caller, Caller->GetPawn(), FVector::ZeroVector))
		return false;

	if (!(CallingPlayer->WorldKillTime == -1) && bConsiderCooldown)
		return false;

	return true;
}

bool ASCPGameState::CanReportPlayer(APlayerController* Caller, ASCPCharacterBase* TargetPawn)
{
	if (!Caller)
		return false;
	if (!TargetPawn)
		return false;

	if (MatchState != MatchState::InProgress || GameplayState != GameplayState::Playing)
		return false;

	ASCPPlayerState* CallingPlayer = Cast<ASCPPlayerState>(Caller->PlayerState);

	if (!CallingPlayer || CallingPlayer->bDead)
		return false;

	if (!TargetPawn->bDead || TargetPawn->bReported)
		return false;


	if (!TargetPawn->IsNetRelevantFor(Caller, Caller->GetPawn(), FVector::ZeroVector))
		return false;

	return true;
}

void ASCPGameState::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

}

void ASCPGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

void ASCPGameState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}

void ASCPGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();

	StateChanged.Broadcast(MatchState, PreviousMatchState); // NO flow dependant code!
}

void ASCPGameState::SetGameplayState(FName NewGameplayState)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		UE_LOG(LogGameState, Log, TEXT("Gameplay State Changed from %s to %s"), *GameplayState.ToString(), *NewGameplayState.ToString());

		GameplayState = NewGameplayState;

		// Call the onrep to make sure the callbacks happen
		OnRep_GameplayState();
	}
}

void ASCPGameState::UpdatedCurrentPlayers()
{
	StateChanged.Broadcast(MatchState, MatchState);
}

void ASCPGameState::OnRep_GameplayState()
{
	StateChanged.Broadcast(GameplayState, MatchState);
}

void ASCPGameState::BroadcastChatMessage_Implementation(ASCPPlayerState* Invoker, const FString& Message)
{
	MessageReceived.Broadcast(Invoker, Message);
}

void ASCPGameState::AddChatMessage(ASCPPlayerState* Invoker, const FString& Message)
{
	if (!Invoker->ChatConfig.bMuted)
		BroadcastChatMessage(Invoker, Message);
}