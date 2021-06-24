// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SCPOnlineSession.h"
#include "Game/SCPGameInstance.h"

void USCPOnlineSession::StartOnlineSession(FName SessionName)
{
	Super::StartOnlineSession(SessionName);

	GetWorld()->GetGameInstance<USCPGameInstance>()->UpdateRichPresence(TEXT("Playing"), TEXT(""), false, false);
}

void USCPOnlineSession::HandleDisconnect(UWorld* World, UNetDriver* NetDriver)
{
	Super::HandleDisconnect(World, NetDriver);

	GetWorld()->GetGameInstance<USCPGameInstance>()->UpdateRichPresence(TEXT("AtMainMenu"), TEXT(""), false, false);
}