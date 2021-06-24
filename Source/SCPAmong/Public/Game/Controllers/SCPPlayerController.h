// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/Controllers/SCPPlayerControllerBase.h"
#include "SCPPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SCPAMONG_API ASCPPlayerController : public ASCPPlayerControllerBase
{
	GENERATED_BODY()

	ASCPPlayerController();

private:
	UFUNCTION()
		virtual void SetupInputComponent() override;

	UFUNCTION()
		virtual void BeginPlay() override;

	UFUNCTION()
		void GameStateChanged(FName NewState, FName PreviousState);

	UFUNCTION()
		TArray<ASCPCharacterBase*> GetSortedPawns();

	void SelectNextCharacter();

	void SelectPreviousCharacter();

	void MainInteract();

	void SecondaryInteract();

private:
	UPROPERTY()
		int32 SelectedIndex;
	UPROPERTY()
		ASCPCharacterBase* SelectedActor;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FWidgetOption WaitingPlayersWidgetClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FWidgetOption HUDWidgetClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FWidgetOption VotingWidgetClass;

public:
	UFUNCTION(BlueprintCallable)
		ASCPCharacterBase* GetSelectedActor() { return SelectedActor; };

	UFUNCTION(BlueprintCallable)
		void SendChatMessage(const FString& Message);

	UFUNCTION(BlueprintCallable)
		void SubmitVote(ASCPPlayerState* Target, bool bSkip);

private:

	UFUNCTION(Unreliable, Server, WithValidation)
		void RequestKill(APlayerController* Caller, ASCPCharacterBase* TargetPawn);

	UFUNCTION(Unreliable, Server, WithValidation)
		void RequestReportBody(APlayerController* Caller, ASCPCharacterBase* TargetPawn);

	UFUNCTION(Unreliable, Server, WithValidation)
		void RequestSendChatMessage(APlayerController* Caller, const FString& Message);

	UFUNCTION(Unreliable, Server, WithValidation)
		void RequestSubmitVote(APlayerController* Caller, bool bSkip, ASCPPlayerState* Target);

	FORCEINLINE int32 Wrap(int32 Val, int32 Max, int32 Min);
};
