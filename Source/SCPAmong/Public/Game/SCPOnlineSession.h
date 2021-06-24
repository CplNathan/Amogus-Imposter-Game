// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/OnlineSession.h"
#include "SCPOnlineSession.generated.h"

/**
 * 
 */
UCLASS()
class SCPAMONG_API USCPOnlineSession : public UOnlineSession
{
	GENERATED_BODY()

	virtual void StartOnlineSession(FName SessionName);
	virtual void HandleDisconnect(UWorld* World, UNetDriver* NetDriver) override;
};
