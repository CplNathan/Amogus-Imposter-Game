// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "SCPGameSession.generated.h"

/**
 * 
 */
UCLASS()
class SCPAMONG_API ASCPGameSession : public AGameSession
{
	GENERATED_BODY()
	
private:
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
};
