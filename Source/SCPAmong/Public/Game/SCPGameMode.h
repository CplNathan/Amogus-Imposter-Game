// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SCPGameMode.generated.h"

class ASCPCharacterBase;
class ASCPPlayerState;
class UVotingManagerServer;

namespace GameplayState
{
	extern SCPAMONG_API const FName Playing;
	extern SCPAMONG_API const FName Voting;
	extern SCPAMONG_API const FName VotingReveal;
}

USTRUCT(BlueprintType)
struct FGameRules
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		int32 MaxPlayers;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		int32 RequiredPlayers;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		int32 RequiredImposters;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		int32 ReadyCountdownStartTime;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		int32 ImposterKillCooldownTime;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		int32 VotingDuration;

	FGameRules()
	{
		MaxPlayers = 16;
		RequiredPlayers = 4;
		RequiredImposters = 1;
		
		ReadyCountdownStartTime = 10;
		ImposterKillCooldownTime = 30;
		VotingDuration = 120;
	}
};

/**
 * 
 */
UCLASS()
class SCPAMONG_API ASCPGameMode : public AGameMode
{
	GENERATED_BODY()

	ASCPGameMode();

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		FGameRules GameRules;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		TSubclassOf<AActor> DeathMarker;

public:
	void KillPlayer(APlayerController* Caller, ASCPCharacterBase* TargetPawn, bool bForce = false);

	void ReportBody(APlayerController* Caller, ASCPCharacterBase* TargetPawn);

	void CallMeeting(APlayerController* Caller);

	void StartVoting(APlayerController* Caller);

	void VotingReveal();

	void StopVoting();

	void SendChatMessage(APlayerController* Caller, const FString& Message);
	
	void SubmitVote(APlayerController* Caller, bool bSkip, ASCPPlayerState* Target);

protected:
	UPROPERTY()
		bool bCountdownReady;

	UPROPERTY()
		FTimerHandle GameReadyHandle;

	UPROPERTY()
		FTimerHandle ImposterKillHandle;

	UPROPERTY()
		UVotingManagerServer* VotingManager;

	// Stores the gameplay state, running/voting just like MatchState.
	FName GameplayState;

	void OnGameplayStateSet();
	void SetGameplayState(FName NewGameplayState);

	virtual void HandleMatchIsWaitingToStart() override;
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleMatchHasStarted() override;
	virtual bool ReadyToEndMatch_Implementation() override;
	virtual void HandleMatchHasEnded() override;

	void MoveToSpectator(AController* Controller, bool bDestroyForce, bool bWantsTearOff = true);

	virtual bool MustSpectate_Implementation(APlayerController* NewPlayerController) const override;

	void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	void BeginPlay() override;
	void PostLogin(APlayerController* NewPlayer) override;
	void Logout(AController* Exiting) override;
	AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	virtual void AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC) override;
	virtual bool FindInactivePlayer(APlayerController* PC) override;

	UPROPERTY()
		TArray<AController*> PlayerControllerList;
};
