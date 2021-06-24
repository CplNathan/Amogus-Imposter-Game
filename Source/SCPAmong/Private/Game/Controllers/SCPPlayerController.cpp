// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Controllers/SCPPlayerController.h"
#include "Game/SCPWorldSettings.h"
#include "Game/SCPCharacterBase.h"
#include "Game/SCPGameMode.h"
#include "Game/SCPPlayerState.h"
#include "Game/SCPGameState.h"
#include "Game/SCPGameInstance.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"

// Sets default values
ASCPPlayerController::ASCPPlayerController()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void ASCPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("NextSelection", IE_Pressed, this, &ASCPPlayerController::SelectNextCharacter);
	InputComponent->BindAction("PreviousSelection", IE_Pressed, this, &ASCPPlayerController::SelectPreviousCharacter);

	InputComponent->BindAction("MainInteract", IE_Pressed, this, &ASCPPlayerController::MainInteract);
	InputComponent->BindAction("SecondaryInteract", IE_Pressed, this, &ASCPPlayerController::SecondaryInteract);
}

void ASCPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ASCPGameState* GameState = Cast<ASCPGameState>(GetWorld()->GetGameState());
	if (!GameState)
		return;

	GameState->StateChanged.AddDynamic(this, &ASCPPlayerController::GameStateChanged);
	GameStateChanged(GameState->GetMatchState(), GameState->GetMatchState());
}

void ASCPPlayerController::GameStateChanged(FName NewState, FName PreviousState)
{
	ASCPGameState* GameState = Cast<ASCPGameState>(GetWorld()->GetGameState());

	if (NewState == MatchState::WaitingToStart || PreviousState == MatchState::EnteringMap)
	{
		if (NewState == MatchState::WaitingToStart && NewState == PreviousState)
		{
			GetWorld()->GetGameInstance<USCPGameInstance>()->UpdateRichPresence(TEXT("WaitingForPlayers"), FString::Printf(TEXT("%02d/%02d"), GameState->CurrentPlayers, GameState->GameRules.MaxPlayers), true, true);
		}

		SetActiveWidget(WaitingPlayersWidgetClass);
	}

	if (NewState == MatchState::InProgress || NewState == GameplayState::Playing)
	{
		GetWorld()->GetGameInstance<USCPGameInstance>()->UpdateRichPresence(TEXT("Playing"), TEXT(""), false, true);
		SetActiveWidget(HUDWidgetClass);

		if (PreviousState == GameplayState::Voting)
		{
			SetIgnoreMoveInput(false);
			SetIgnoreLookInput(false);
		}
	}
	if (NewState == GameplayState::Voting)
	{
		GetWorld()->GetGameInstance<USCPGameInstance>()->UpdateRichPresence(TEXT("Voting"), TEXT(""), false, true);
		SetActiveWidget(VotingWidgetClass);

		if (NewState == MatchState::InProgress || NewState == GameplayState::Playing)
		{
			SetIgnoreMoveInput(true);
			SetIgnoreLookInput(true);
		}
	}
	else if (NewState == MatchState::WaitingPostMatch)
	{
		GetWorld()->GetGameInstance<USCPGameInstance>()->UpdateRichPresence(TEXT("GameOver"), TEXT(""), false, true);
		SetActiveWidget(FWidgetOption());
	}
	else if (NewState == MatchState::LeavingMap)
	{
		SetActiveWidget(FWidgetOption());
	}
}

TArray<ASCPCharacterBase*> ASCPPlayerController::GetSortedPawns()
{
	APawn* LocalPawn = GetPawn();
	if (!LocalPawn)
		return TArray<ASCPCharacterBase*>{};

	AGameState* GameState = Cast<AGameState>(GetWorld()->GetGameState());
	if (!GameState)
		return TArray<ASCPCharacterBase*>{};

	TArray<ASCPCharacterBase*> Pawns;
	for (TActorIterator<ASCPCharacterBase> It(GetWorld(), ASCPCharacterBase::StaticClass()); It; ++It)
	{
		ASCPCharacterBase* Actor = *It;
		Pawns.Add(Actor);
	}

	Pawns = Pawns.FilterByPredicate([LocalPawn, this](const ASCPCharacterBase* v1) // Remove invalid pawns (ones out of range - not being streamed)
	{
		if (!v1->IsValidLowLevel())
			return false;

		return !(v1 == LocalPawn) && v1->IsNetRelevantFor(this, LocalPawn, FVector::ZeroVector);
	});

	FVector SourceLocation = LocalPawn->GetActorLocation();
	Pawns.Sort([SourceLocation](const ASCPCharacterBase& v1, const ASCPCharacterBase& v2)
	{
		float DistanceA = FVector::DistSquared(SourceLocation, v1.GetActorLocation());
		float DistanceB = FVector::DistSquared(SourceLocation, v2.GetActorLocation());

		return DistanceA > DistanceB;
	});

	return Pawns;
}

void ASCPPlayerController::SelectNextCharacter()
{
	TArray<ASCPCharacterBase*> Pawns = GetSortedPawns();
	if (Pawns.Num() == 0)
		return;

	SelectedIndex = Wrap((SelectedIndex + 1), Pawns.Num() - 1, 0);
	SelectedActor = Pawns[SelectedIndex];
}

void ASCPPlayerController::SelectPreviousCharacter()
{
	TArray<ASCPCharacterBase*> Pawns = GetSortedPawns();
	if (Pawns.Num() == 0)
		return;

	SelectedIndex = Wrap((SelectedIndex - 1), Pawns.Num() - 1, 0);
	SelectedActor = Pawns[SelectedIndex];
}

void ASCPPlayerController::MainInteract()
{
	ASCPGameState* GameState = GetWorld()->GetGameState<ASCPGameState>();
	if (!GameState)
		return;

	if (GameState->CanKillPlayer(this, SelectedActor))
	{
		ASCPPlayerState* SCPPlayerState = SelectedActor->GetPlayerState<ASCPPlayerState>();
		if (SCPPlayerState && GetNetMode() == ENetMode::NM_Client) // Only set prediction if the predicted object is valid and we are running on a client, if this is a listen server values will be updated immediately in gamemode.
		{
			GetPlayerState<ASCPPlayerState>()->WorldKillTime = GameState->GetServerWorldTimeSeconds() + GameState->GameRules.ImposterKillCooldownTime; // prerepl
			SCPPlayerState->bDead = true; // Set dead true now so we dont wait for delayed replication - if this is wrong it will be eventually corrected.
			SCPPlayerState->ReplPawnData();
		}

		RequestKill(this, SelectedActor);
	}
}

void ASCPPlayerController::SecondaryInteract()
{
	ASCPGameState* GameState = GetWorld()->GetGameState<ASCPGameState>();
	if (!GameState)
		return;

	if (GameState->CanReportPlayer(this, SelectedActor))
	{
		ASCPCharacterBase* InPlayerPawn = Cast<ASCPCharacterBase>(SelectedActor);
		if (InPlayerPawn && GetNetMode() == ENetMode::NM_Client) // Only set prediction if the predicted object is valid and we are running on a client, if this is a listen server values will be updated immediately in gamemode.
		{
			InPlayerPawn->bReported = true;
		}

		RequestReportBody(this, SelectedActor);
	}
}

void ASCPPlayerController::SendChatMessage(const FString& Message)
{
	ASCPPlayerState* SCPPlayerState = GetPlayerState<ASCPPlayerState>();
	if (!SCPPlayerState)
		return;

	if (SCPPlayerState->CanSendChatMessage(Message, GetWorld()->GetRealTimeSeconds()) && GetNetMode() == ENetMode::NM_Client) // Only set prediction if the predicted object is valid and we are running on a client, if this is a listen server values will be updated immediately in gamemode.
	{
		SCPPlayerState->ChatConfig.LastMessageSent = GetWorld()->GetRealTimeSeconds(); // probably dont set this if host
	}

	RequestSendChatMessage(this, Message);
}

void ASCPPlayerController::SubmitVote(ASCPPlayerState* Target, bool bSkip)
{
	ASCPPlayerState* SCPPlayerState = GetPlayerState<ASCPPlayerState>();
	if (!SCPPlayerState)
		return;

	if (SCPPlayerState->CanSubmitVote(Target, bSkip) && GetNetMode() == ENetMode::NM_Client) // Only set prediction if the predicted object is valid and we are running on a client, if this is a listen server values will be updated immediately in gamemode.
	{
		SCPPlayerState->MeetingData.bVoted = true;
		SCPPlayerState->MeetingData.bSkipped = bSkip || (Target == nullptr);
	}

	RequestSubmitVote(this, bSkip, Target);
}

void ASCPPlayerController::RequestKill_Implementation(APlayerController* Caller, ASCPCharacterBase* TargetPawn)
{
	if (!RequestKill_Validate(Caller, TargetPawn))
		return;

	ASCPGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCPGameMode>();
	GameMode->KillPlayer(Caller, TargetPawn);
}

bool ASCPPlayerController::RequestKill_Validate(APlayerController* Caller, ASCPCharacterBase* TargetPawn)
{
	ASCPGameState* GameState = GetWorld()->GetGameState<ASCPGameState>();
	if (!GameState)
		return false;

	return GameState->CanKillPlayer(Caller, TargetPawn);
}

void ASCPPlayerController::RequestReportBody_Implementation(APlayerController* Caller, ASCPCharacterBase* TargetPawn)
{
	if (!RequestReportBody_Validate(Caller, TargetPawn))
		return;

	ASCPGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCPGameMode>();
	GameMode->ReportBody(Caller, TargetPawn);
}

bool ASCPPlayerController::RequestReportBody_Validate(APlayerController* Caller, ASCPCharacterBase* TargetPawn)
{
	ASCPGameState* GameState = GetWorld()->GetGameState<ASCPGameState>();
	if (!GameState)
		return false;

	return GameState->CanReportPlayer(Caller, TargetPawn);
}

void ASCPPlayerController::RequestSendChatMessage_Implementation(APlayerController* Caller, const FString& Message)
{
	if (!RequestSendChatMessage_Validate(Caller, Message))
		return;

	ASCPGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCPGameMode>();
	GameMode->SendChatMessage(Caller, Message);
}

bool ASCPPlayerController::RequestSendChatMessage_Validate(APlayerController* Caller, const FString& Message)
{
	ASCPPlayerState* SCPPlayerState = Caller->GetPlayerState<ASCPPlayerState>();
	if (!SCPPlayerState)
		return false;

	return SCPPlayerState->CanSendChatMessage(Message, GetWorld()->GetRealTimeSeconds());
}

void ASCPPlayerController::RequestSubmitVote_Implementation(APlayerController* Caller, bool bSkip, ASCPPlayerState* Target)
{
	if (!RequestSubmitVote_Validate(Caller, bSkip, Target))
		return;

	ASCPGameMode* GameMode = GetWorld()->GetAuthGameMode<ASCPGameMode>();
	GameMode->SubmitVote(Caller, bSkip, Target);
}

bool ASCPPlayerController::RequestSubmitVote_Validate(APlayerController* Caller, bool bSkip, ASCPPlayerState* Target)
{
	ASCPPlayerState* SCPPlayerState = Caller->GetPlayerState<ASCPPlayerState>();
	if (!SCPPlayerState)
		return false;

	return SCPPlayerState->CanSubmitVote(Target, bSkip);
}

int32 ASCPPlayerController::Wrap(int32 Val, int32 Max, int32 Min)
{
	int range_size = Max - Min + 1;

	if (Val < Min)
		Val += range_size * ((Min - Val) / range_size + 1);

	return Min + (Val - Min) % range_size;
}