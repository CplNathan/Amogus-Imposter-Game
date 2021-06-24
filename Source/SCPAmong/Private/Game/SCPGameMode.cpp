// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SCPGameMode.h"
#include "Game/SCPGameState.h"
#include "Game/Controllers/SCPPlayerController.h"
#include "Game/SCPGameSession.h"
#include "Game/SCPCharacterBase.h"
#include "Game/Voting/VotingManagerServer.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerStart.h"
#include "Game/SCPPlayerState.h"
#include "Engine/StaticMeshActor.h"
#include "Components/CapsuleComponent.h"
#include "AdvancedFriendsLibrary.h"
#include "EngineUtils.h"

namespace GameplayState
{
	const FName Playing = FName(TEXT("InProgress"));
	const FName Voting = FName(TEXT("Voting"));
	const FName VotingReveal = FName(TEXT("VotingReveal"));
}

ASCPGameMode::ASCPGameMode()
{
	/* Game Mode Settings */
	bStartPlayersAsSpectators = true;
	bPauseable = true; // maybe

	GameStateClass = ASCPGameState::StaticClass();
	PlayerStateClass = ASCPPlayerState::StaticClass();
	PlayerControllerClass = ASCPPlayerController::StaticClass();
	GameSessionClass = ASCPGameSession::StaticClass();
	// TODO Native spectator class!
}

void ASCPGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	PlayerControllerList.AddUnique((AController*)NewPlayer);

	ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();
	if (SCPGameState) // may not exist yet
		SCPGameState->CurrentPlayers = PlayerControllerList.Num(); // Todo consolidate into a gamerules class
}

void ASCPGameMode::Logout(AController* Exiting)
{
	PlayerControllerList.Remove(Exiting);

	GetGameState<ASCPGameState>()->CurrentPlayers = PlayerControllerList.Num(); // Todo consolidate into a gamerules class
}

void ASCPGameMode::AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC)
{
	Super::AddInactivePlayer(PlayerState, PC);

	ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();
	SCPGameState->InactivePlayerArray = InactivePlayerArray;

	ASCPPlayerState* SCPPlayerState = Cast<ASCPPlayerState>(PlayerState);
	SCPPlayerState->bDead = true;
	SCPPlayerState->ReplPawnData();
}

bool ASCPGameMode::FindInactivePlayer(APlayerController* PC)
{
	bool bFound = Super::FindInactivePlayer(PC);

	if (bFound)
	{
		ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();
		SCPGameState->InactivePlayerArray = InactivePlayerArray;
	}

	return bFound;
}

void ASCPGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (NumPlayers >= GameRules.MaxPlayers)
		ErrorMessage = "Session is full";

	FJoinabilitySettings Joinability;
	GameSession->GetSessionJoinability(GameSession->SessionName, Joinability);

	if (Joinability.bJoinViaPresenceFriendsOnly)
	{
		IOnlineFriendsPtr FriendsInterface = Online::GetFriendsInterface();
		if (FriendsInterface.IsValid())
		{
			bool bHasFriend = false;
			for (AController* Controller : PlayerControllerList)
			{
				APlayerController* PlayerController = Cast<APlayerController>(Controller);
				FBPUniqueNetId FBPUniqueId;
				FBPUniqueId.SetUniqueNetId(UniqueId.GetUniqueNetId());

				bool bIsFriend = false;
				//UAdvancedFriendsLibrary::IsAFriend(PlayerController, FBPUniqueId, bIsFriend); // Symbol error

				if (bIsFriend)
				{
					bHasFriend = true;
					break;
				}
			}

			if (!bHasFriend)
				ErrorMessage = "No friends in session";
		}
	}
}

AActor* ASCPGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Choose a player start
	APlayerStart* FoundPlayerStart = nullptr;
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	TArray<APlayerStart*> StartPoints;
	UWorld* World = GetWorld();
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		StartPoints.Add(PlayerStart);
	}

	if (FoundPlayerStart == nullptr)
	{
		if (StartPoints.Num() > 0)
		{
			FoundPlayerStart = StartPoints[FMath::RandRange(0, StartPoints.Num() - 1)];
		}
	}
	return FoundPlayerStart;
}

void ASCPGameMode::BeginPlay()
{
	Super::BeginPlay();

	ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();
	SCPGameState->GameRules = GameRules;
}

void ASCPGameMode::KillPlayer(APlayerController* Caller, ASCPCharacterBase* TargetPawn, bool bForce)
{
	float TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(ImposterKillHandle);

	ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();

	if ((!SCPGameState->CanKillPlayer(Caller, TargetPawn) || TimeRemaining > 0) && !bForce)
		return;

	ASCPPlayerState* TargetState = TargetPawn->GetPlayerState<ASCPPlayerState>();
	if (TargetState)
	{
		TargetState->bDead = true;
		TargetState->ReplPawnData(); // Ensure replication to pawn (even for host).
	}

	MoveToSpectator(TargetPawn->GetController(), bForce, bForce); // DESTROY PAWN! And then spawn a replacement radgol that takes a skeletal mesh, that ragdol should inherit from a new class called FogVisibilityActor that has the isnetrelevant and hidden functionality... 

	FTimerDelegate ResetKillTime;
	ResetKillTime.BindLambda([this]
	{
		ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();

		for (APlayerState* Player : SCPGameState->GetAllPlayers())
		{
			ASCPPlayerState* SCPPlayer = Cast<ASCPPlayerState>(Player);
			if (SCPPlayer->bIsImposter)
				SCPPlayer->WorldKillTime = -1;
		}
	});

	GetWorld()->GetTimerManager().SetTimer(ImposterKillHandle, ResetKillTime, GameRules.ImposterKillCooldownTime, false);

	for (APlayerState* Player : SCPGameState->GetAllPlayers())
	{
		ASCPPlayerState* SCPPlayer = Cast<ASCPPlayerState>(Player);
		if (SCPPlayer->bIsImposter)
		{
			SCPPlayer->WorldKillTime = GameState->GetServerWorldTimeSeconds() + GameRules.ImposterKillCooldownTime;
			SCPPlayer->Kills.Add(TargetState);
		}
	}
}

void ASCPGameMode::ReportBody(APlayerController* Caller, ASCPCharacterBase* TargetPawn)
{

	ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();

	if (!SCPGameState->CanReportPlayer(Caller, TargetPawn))
		return;

	ASCPCharacterBase* SCPTargetPawn = Cast<ASCPCharacterBase>(TargetPawn);
	if (SCPTargetPawn)
	{
		SCPTargetPawn->bReported = true;
	}

	StartVoting(Caller); // TODO pass caller
}

void ASCPGameMode::CallMeeting(APlayerController* Caller)
{
	// todo checks
	StartVoting(Caller);
}

void ASCPGameMode::StartVoting(APlayerController* Caller)
{
	if (GameplayState == GameplayState::Playing)
	{
		ASCPPlayerState* SCPPlayer = Caller->GetPlayerState<ASCPPlayerState>();
		VotingManager = UVotingManagerServer::UVotingManagerServerSetup(this, GameRules);
		VotingManager->StartVoting(SCPPlayer);

		SetGameplayState(GameplayState::Voting);
	}
}

void ASCPGameMode::VotingReveal()
{
	if (GameplayState == GameplayState::Voting)
	{
		SetGameplayState(GameplayState::VotingReveal);
	}
}

void ASCPGameMode::StopVoting()
{
	if (GameplayState == GameplayState::VotingReveal)
	{
		ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();

		for (APlayerState* PlayerState : SCPGameState->GetAlivePlayers())
		{
			APawn* PlayerPawn = PlayerState->GetPawn();

			if (PlayerPawn)
			{
				AController* PlayerController = PlayerPawn->GetController();

				PlayerPawn->SetActorLocation(FindPlayerStart(PlayerController)->GetActorLocation());
			}
		}

		for (TActorIterator<APawn> It(GetWorld()); It; ++It)
		{
			APawn* Pawn = *It;
			ASCPCharacterBase* SCPCharacter = Cast<ASCPCharacterBase>(Pawn);

			if (SCPCharacter)
			{
				if (SCPCharacter->bDead && !SCPCharacter->IsPendingKill())
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

					FVector SpawnLocation = SCPCharacter->GetActorLocation();
					SpawnLocation.Z = SCPCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					AActor* DeathMarkerActor = GetWorld()->SpawnActor<AActor>(DeathMarker, SpawnLocation, SCPCharacter->GetActorRotation(), SpawnParams);
					SCPCharacter->Destroy();
				}
			}
		}

		SetGameplayState(MatchState);
	}
}

void ASCPGameMode::SendChatMessage(APlayerController* Caller, const FString& Message)
{
	ASCPPlayerState* PlayerState = Caller->GetPlayerState<ASCPPlayerState>();
	if (!PlayerState)
		return;

	if (!PlayerState->CanSendChatMessage(Message, GetWorld()->GetRealTimeSeconds()))
		return;

	PlayerState->ChatConfig.LastMessageSent = GetWorld()->GetRealTimeSeconds();

	ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();
	SCPGameState->AddChatMessage(PlayerState, Message);
}

void ASCPGameMode::SubmitVote(APlayerController* Caller, bool bSkip, ASCPPlayerState* Target) //todo skip
{
	ASCPPlayerState* PlayerState = Caller->GetPlayerState<ASCPPlayerState>();
	if (!PlayerState)
		return;

	if (!PlayerState->CanSubmitVote(Target, bSkip))
		return;

	VotingManager->SubmitVote(PlayerState, bSkip, Target);
}

void ASCPGameMode::SetGameplayState(FName NewGameplayState)
{
	GameplayState = NewGameplayState;

	OnGameplayStateSet();

	GetGameState<ASCPGameState>()->SetGameplayState(NewGameplayState);
}

void ASCPGameMode::OnGameplayStateSet()
{
	if (GameplayState == GameplayState::Playing)
	{
		TArray<APlayerState*> AllPlayers = GetGameState<ASCPGameState>()->PlayerArray;

		IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface();

		if (VoiceInterface.IsValid())
		{
			for (APlayerState* PlayerState : AllPlayers)
			{
				VoiceInterface->UnregisterRemoteTalker(*PlayerState->GetUniqueId().GetUniqueNetId());
			}

			for (AController* Controller : PlayerControllerList)
			{
				APlayerController* PC = Cast<APlayerController>(Controller);
				//PC->SetPause(false); // NOPE look into IgnoreMoveInput in Controller - also make sure to add checks to all functions (like kill, vote etc) that we are in the correct gamestate! - maybe disable replication for the pawn during this time etc
			}
		}
	}
	else if (GameplayState == GameplayState::Voting)
	{
		TArray<APlayerState*> AllPlayers = GetGameState<ASCPGameState>()->PlayerArray;

		IOnlineVoicePtr VoiceInterface = Online::GetVoiceInterface();

		if (VoiceInterface.IsValid())
		{
			for (APlayerState* PlayerState : AllPlayers)
			{
				ASCPPlayerState* SCPPlayerState = Cast<ASCPPlayerState>(PlayerState);
				if (!SCPPlayerState->bDead)
					VoiceInterface->RegisterRemoteTalker(*PlayerState->GetUniqueId().GetUniqueNetId());
			}

			for (AController* Controller : PlayerControllerList)
			{
				APlayerController* PC = Cast<APlayerController>(Controller);
				//PC->SetPause(true); // NOPE look into IgnoreMoveInput in Controller - also make sure to add checks to all functions (like kill, vote etc) that we are in the correct gamestate! - maybe disable replication for the pawn during this time etc
			}
		}
	}
}

void ASCPGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

}

bool ASCPGameMode::ReadyToStartMatch_Implementation()
{
	float TimeRemaining = GetWorld()->GetTimerManager().GetTimerRemaining(GameReadyHandle);

	if (NumPlayers >= GameRules.RequiredPlayers && TimeRemaining == -1)
	{
		FTimerDelegate SetGameReady;
		SetGameReady.BindLambda([this]
		{
			ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();
			SCPGameState->WorldReadyTime = -1;

			bCountdownReady = true;
		});

		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(GameReadyHandle, SetGameReady, GameRules.ReadyCountdownStartTime, false);

		ASCPGameState* SCPGameState = GetGameState<ASCPGameState>();
		SCPGameState->WorldReadyTime = GameState->GetServerWorldTimeSeconds() + GameRules.ReadyCountdownStartTime;
	}
	// Replicated in Tick

	return (bCountdownReady && NumPlayers >= GameRules.RequiredPlayers);
}

void ASCPGameMode::HandleMatchHasStarted()
{
	TArray<APlayerState*> AllPlayers = GetGameState<ASCPGameState>()->GetAllPlayers();

	for (APlayerState* PlayerState : AllPlayers)
	{
		ASCPPlayerState* SCPPlayer = Cast<ASCPPlayerState>(PlayerState);
		SCPPlayer->bPlaying = true;
		SCPPlayer->Colour = FLinearColor::MakeRandomColor();
	}

	TArray<int32> Imposters;
	for (int32 i = 0; i < GameRules.RequiredImposters; i++)
	{
		int32 RandomIndex = FMath::RandRange(0, AllPlayers.Num() - 1);
		Imposters.Add(RandomIndex);
	}

	for (int32 i : Imposters)
	{
		ASCPPlayerState* NewImposter = Cast<ASCPPlayerState>(AllPlayers[i]);

		NewImposter->bIsImposter = true;

		for (int32 oldi : Imposters)
		{
			ASCPPlayerState* OldImposter = Cast<ASCPPlayerState>(AllPlayers[oldi]);
			OldImposter->Imposters.Add(NewImposter);
		}
	}

	Super::HandleMatchHasStarted(); // this handles the restartplayer - maybe override if we want
	SetGameplayState(MatchState);

	for (APlayerState* PlayerState : AllPlayers)
	{
		ASCPPlayerState* SCPPlayer = Cast<ASCPPlayerState>(PlayerState);
		SCPPlayer->ReplPawnData(); // Ensure replication to pawn (even for host).
	}
}

bool ASCPGameMode::ReadyToEndMatch_Implementation()
{
	TArray<APlayerState*> AllPlayers = GetGameState<ASCPGameState>()->PlayerArray; // If players disconnect this doesent get triggered properly

	int32 AliveImposters = 0;
	int32 AliveCrew = 0;
	for (APlayerState* Player : AllPlayers)
	{
		ASCPPlayerState* SCPPlayer = Cast<ASCPPlayerState>(Player);

		if (SCPPlayer->bDead || SCPPlayer->IsInactive() || InactivePlayerArray.Contains(SCPPlayer))
			continue;

		if (SCPPlayer->bIsImposter)
			AliveImposters++;
		else
			AliveCrew++;
	}

	return AliveImposters >= AliveCrew || AliveImposters == 0;
}

void ASCPGameMode::HandleMatchHasEnded()
{
	for (AController* Controller : PlayerControllerList)
	{
		MoveToSpectator(Controller, true); // TODO: More gracfully
	}

	Super::HandleMatchHasEnded();

	RestartGame();
}

void ASCPGameMode::MoveToSpectator(AController* Controller, bool bDestroyForce, bool bWantsTearOff) // TODO: More gracfully
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController)
	{
		APawn* Pawn = PlayerController->GetPawn();
		if (Pawn)
		{
			Pawn->SetReplicatingMovement(false);
			if (bWantsTearOff)
				Pawn->TearOff();

			PlayerController->UnPossess();
			PlayerController->PlayerState->SetIsSpectator(true);
			PlayerController->ClientGotoState(NAME_Spectating);
			PlayerController->ChangeState(NAME_Spectating);

			if (bDestroyForce)
				Pawn->Destroy(true);
			else
				Pawn->SetActorTickEnabled(false);
		}
	}
}

bool ASCPGameMode::MustSpectate_Implementation(APlayerController* NewPlayerController) const
{
	//return IsMatchInProgress();
	return false;
}