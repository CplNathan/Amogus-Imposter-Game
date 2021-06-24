// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Game/SCPGameMode.h"
#include "VotingManagerServer.generated.h"

class ASCPPlayerState;

USTRUCT()
struct FVote
{
	GENERATED_BODY()

	UPROPERTY()
		ASCPPlayerState* Votee;

	UPROPERTY()
		TArray<ASCPPlayerState*> Voters;
};

/**
 * Abstracted wrapper class for handling the various timers and callbacks for the voting portion of an imposter game.
 */
UCLASS()
class SCPAMONG_API UVotingManagerServer : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
		static UVotingManagerServer* UVotingManagerServerSetup(ASCPGameMode* Outer, FGameRules InRules);

	UFUNCTION()
		void StartVoting(ASCPPlayerState* Caller);

	UFUNCTION()
		void SubmitVote(ASCPPlayerState* PlayerState, bool bSkip, ASCPPlayerState* Target);

private:
	FGameRules Rules;

	ASCPPlayerState* MeetingCaller;

	TArray<FVote> Votes;
protected:
	UFUNCTION()
		void Init(FGameRules InRules);

	UFUNCTION()
		void StartRevealTime();

	UFUNCTION()
		void VotingEnded();

	UPROPERTY()
		FTimerHandle VotingTimerHandle;

	UPROPERTY()
		FTimerHandle RevealTimerHandle;
};
