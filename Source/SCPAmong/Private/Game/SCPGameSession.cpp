// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SCPGameSession.h"
#include "Net/OnlineEngineInterface.h"

void ASCPGameSession::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

}

void ASCPGameSession::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	UOnlineEngineInterface::Get()->UpdateSessionJoinability(GetWorld(), SessionName, false, false, false, false);
}

void ASCPGameSession::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	UOnlineEngineInterface::Get()->UpdateSessionJoinability(GetWorld(), SessionName, true, true, true, true); // todo update with desired game settings (invite only etc)
}