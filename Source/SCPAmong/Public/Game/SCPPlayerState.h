// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SCPPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRevealVotedBy, const TArray<ASCPPlayerState*>&, VotedBy);

USTRUCT(BlueprintType)
struct FMeetingData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly)
		bool bCaller;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly)
		bool bVoted;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly)
		bool bSkipped;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly)
		TArray<ASCPPlayerState*> VotedBy;

	FMeetingData()
	{
		bCaller = false;
		bVoted = false;
		bSkipped = false;
	}
};

USTRUCT(BlueprintType)
struct FUserChatConfig
{
	GENERATED_BODY()

	int32 TimeBetweenMessages;
	int32 CharacterLimit;
	bool bMuted;

	// Get Real Time Seconds
	float LastMessageSent;

	FUserChatConfig()
	{
		TimeBetweenMessages = 1;
		CharacterLimit = 128;
		bMuted = false;

		LastMessageSent = 0;
	}
};

/**
 * 
 */
UCLASS()
class SCPAMONG_API ASCPPlayerState : public APlayerState
{
	GENERATED_BODY()

		ASCPPlayerState();

public:
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated)
		FUserChatConfig ChatConfig;
	UFUNCTION(BlueprintCallable)
		bool CanSendChatMessage(const FString& Message, float RealTimeSeconds, bool bIgnoreCooldown = false);
	UFUNCTION(BlueprintCallable)
		bool CanSubmitVote(ASCPPlayerState* Victim, bool bSkip);

public:
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated)
		bool bPlaying;
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated)
		FLinearColor Colour;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated, ReplicatedUsing = UpdateMeetingData)
		FMeetingData MeetingData;

	// Takes into consideration if the caller is the server, if client is a server then bIsImposter would be known when it shouldnt be, with this function it is taken into account.
	UFUNCTION(BlueprintCallable)
		bool GetIsImposterSafe() { return (bIsImposter && (GetWorld()->GetFirstPlayerController()->GetPlayerState<ASCPPlayerState>()->bIsImposter /* || bIsExposedImposter */)); };

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated)
		bool bIsImposter;
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated, ReplicatedUsing = UpdateImposters)
		TArray<APlayerState*> Imposters;
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated, ReplicatedUsing = UpdateImposterKills)
		TArray<APlayerState*> Kills;
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated)
		bool bDead;

	UPROPERTY(BlueprintAssignable)
		FRevealVotedBy RevealVoted;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
		int32 WorldKillTime;

	UFUNCTION()
		void ReplPawnData();

	UFUNCTION()
		void UpdateMeetingData(FMeetingData OldData);

	UFUNCTION()
		void UpdateImposters();
	UFUNCTION()
		void UpdateImposterKills();
	
protected:
	virtual void BeginPlay() override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
	void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OverrideWith(APlayerState* PlayerState) override;
};
