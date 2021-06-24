// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Voting/VotingManagerServer.h"
#include "Game/SCPGameState.h"
#include "Game/SCPPlayerState.h"

UVotingManagerServer* UVotingManagerServer::UVotingManagerServerSetup(ASCPGameMode* Outer, FGameRules InRules)
{
	UVotingManagerServer* NewVotingManager = NewObject<UVotingManagerServer>(Outer);
	NewVotingManager->Init(InRules);
	return NewVotingManager;
}

void UVotingManagerServer::Init(FGameRules InRules)
{
	Rules = InRules;
}

void UVotingManagerServer::StartVoting(ASCPPlayerState* Caller)
{
	ASCPGameState* GameState = GetWorld()->GetGameState<ASCPGameState>();

	for (APlayerState* Player : GameState->GetAlivePlayers())
	{
		ASCPPlayerState* SCPPlayer = Cast<ASCPPlayerState>(Player);
		SCPPlayer->MeetingData = FMeetingData();

		FVote NewVote;
		NewVote.Votee = SCPPlayer;
		Votes.Add(NewVote);
	}

	FTimerDelegate StartRevealTimeDelegate;
	StartRevealTimeDelegate.BindLambda([this]
	{
		StartRevealTime();
	});

	int32 InitialTime = 10;
	GetWorld()->GetTimerManager().SetTimer(VotingTimerHandle, StartRevealTimeDelegate, InitialTime, false);

	GameState->WorldVotingTime = GameState->GetServerWorldTimeSeconds() + InitialTime;
	MeetingCaller = Caller;
	MeetingCaller->MeetingData.bCaller = true;
}

void UVotingManagerServer::StartRevealTime()
{
	FTimerDelegate EndVotingTimeDelegate;
	EndVotingTimeDelegate.BindLambda([this]
	{
		VotingEnded();
	});

	GetWorld()->GetTimerManager().SetTimer(RevealTimerHandle, EndVotingTimeDelegate, Rules.VotingDuration, false);

	for (FVote& Vote : Votes)
	{
		FMeetingData OldData = Vote.Votee->MeetingData;
		Vote.Votee->MeetingData.VotedBy = Vote.Voters;
		Vote.Votee->UpdateMeetingData(OldData); // ensure callback is fired on host
	}

	ASCPGameMode* GameMode = Cast<ASCPGameMode>(GetOuter());
	GameMode->VotingReveal();
}

void UVotingManagerServer::VotingEnded()
{
	ASCPGameState* GameState = GetWorld()->GetGameState<ASCPGameState>();
	GameState->WorldVotingTime = -1;

	MeetingCaller->MeetingData.bCaller = false;
	MeetingCaller = nullptr;

	Votes.Sort([](const FVote& v1, const FVote& v2) {
		return v1.Voters.Num() > v2.Voters.Num();
	});

	int32 Skips = 0;
	for (ASCPPlayerState* Player : GameState->GetAlivePlayers())
	{
		if (Player->MeetingData.bSkipped || !Player->MeetingData.bVoted)
			Skips++;
	}

	bool bSkipCondemn = false;

	if (Votes.Num() >= 1)
	{
		if (Skips == Votes[0].Voters.Num())
			bSkipCondemn = true;
	}

	if (Votes.Num() >= 2 && !bSkipCondemn)
	{
		if (Votes[0].Voters.Num() == Votes[1].Voters.Num())
			bSkipCondemn = true;
	}

	ASCPGameMode* GameMode = Cast<ASCPGameMode>(GetOuter());
	if (!bSkipCondemn)
		GameMode->KillPlayer(nullptr, (ASCPCharacterBase*)Votes[0].Votee->GetPawn(), true);

	Votes.Empty();

	GameMode->StopVoting();
}

void UVotingManagerServer::SubmitVote(ASCPPlayerState* PlayerState, bool bSkip, ASCPPlayerState* Target)
{
	ASCPGameState* GameState = GetWorld()->GetGameState<ASCPGameState>();

	FVote* PlayerVotes = Votes.FindByPredicate([Target](FVote& v1) {
		return v1.Votee == Target;
	});

	if (PlayerVotes && !bSkip)
		PlayerVotes->Voters.Add(PlayerState);

	PlayerState->MeetingData.bVoted = true;
	PlayerState->MeetingData.bSkipped = bSkip;

	if (GameState->GetVotedPlayers().Num() >= GameState->GetAlivePlayers().Num())
	{
		GetWorld()->GetTimerManager().ClearTimer(RevealTimerHandle);
		VotingEnded();
	};
}