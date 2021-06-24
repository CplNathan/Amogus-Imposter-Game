// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Game/SCPGameMode.h"
#include "SCPGameState.generated.h"

class ASCPPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGameStateChanged, FName, MatchState, FName, PreviousState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMessageReceived, ASCPPlayerState*, Invoker, const FString&, Message);

/**
 * 
 */
UCLASS()
class SCPAMONG_API ASCPGameState : public AGameState
{
	GENERATED_BODY()

		ASCPGameState();

public:
	UPROPERTY(Replicated)
		TArray<APlayerState*> InactivePlayerArray;

public:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = UpdatedCurrentPlayers)
		int32 CurrentPlayers;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
		FGameRules GameRules;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
		int32 WorldReadyTime;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
		int32 WorldVotingTime;

	UFUNCTION(BlueprintCallable)
		int32 GetTimeRemainingWorld(int32 TimeToCompare, int32 DefaultTime) { return TimeToCompare == -1 ? DefaultTime : TimeToCompare < GetServerWorldTimeSeconds() ? 0 : TimeToCompare - GetServerWorldTimeSeconds(); };

public:
	UFUNCTION(BlueprintCallable)
		bool CanKillPlayer(APlayerController* Caller, ASCPCharacterBase* TargetPawn, bool bConsiderCooldown = true);
	UFUNCTION(BlueprintCallable)
		bool CanReportPlayer(APlayerController* Caller, ASCPCharacterBase* TargetPawn);
	UFUNCTION(BlueprintCallable)
		TArray<APlayerState*> GetAllPlayers() { TArray<APlayerState*> AllPlayers; AllPlayers.Append(PlayerArray); AllPlayers.Append(InactivePlayerArray); return AllPlayers; };
	UFUNCTION(BlueprintCallable)
		TArray<ASCPPlayerState*> GetAlivePlayers();
	UFUNCTION(BlueprintCallable)
		TArray<ASCPPlayerState*> GetVotedPlayers();
	UFUNCTION(BlueprintCallable)
		TArray<ASCPPlayerState*> GetNotVotedPlayers();
	UFUNCTION(BlueprintCallable)
		TArray<APlayerState*> GetInactivePlayers() { return InactivePlayerArray; };
	UFUNCTION(BlueprintCallable)
		TArray<APlayerState*> GetActivePlayers() { return PlayerArray; };

private:
	virtual void HandleMatchIsWaitingToStart() override;

	virtual void HandleMatchHasStarted() override;

	virtual void HandleMatchHasEnded() override;

	virtual void OnRep_MatchState() override;

public:
	UFUNCTION()
		void SetGameplayState(FName NewGameplayState);

	UFUNCTION(BlueprintCallable)
		FName GetGameplayState() { return GameplayState; };

	UPROPERTY(BlueprintAssignable)
		FGameStateChanged StateChanged;

private:
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_GameplayState)
		FName GameplayState;

	UFUNCTION()
		void UpdatedCurrentPlayers();

	UFUNCTION()
		void OnRep_GameplayState();
public:
	UFUNCTION()
		bool IsVotingTime() { return GameplayState == GameplayState::Voting; };

protected:
	void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

private:
	UFUNCTION(NetMulticast, Unreliable)
		void BroadcastChatMessage(ASCPPlayerState* Invoker, const FString& Message);

public:
	UFUNCTION()
		void AddChatMessage(ASCPPlayerState* Invoker, const FString& Message);

	UPROPERTY(BlueprintAssignable)
		FMessageReceived MessageReceived;
};
