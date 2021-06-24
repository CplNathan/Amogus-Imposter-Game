// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SCPGameInstance.generated.h"

class UOnlineSession;

/**
 * 
 */
UCLASS()
class SCPAMONG_API USCPGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;

	virtual TSubclassOf<UOnlineSession> GetOnlineSessionClass() override;
	void OnPresencePushComplete(const class FUniqueNetId& UniqueId, const bool bSuccess);

	/*
	"lang"
	{
		"language"	"english"
		"tokens"
		{
			"#StatusWithoutScore"	"{#Status_%gamestatus%}"
			"#StatusWithScore"	"{#Status_%gamestatus%}: %SCORE%"
			"#Status_AtMainMenu"	"At the main menu"
			"#Status_WaitingForMatch"	"Waiting for match"
			"#Status_WaitingForPlayers"	"Waiting for players"
			"#Status_Voting"	"Voting"
			"#Status_GameOver"	"Game over"
		}
	}
	*/
	void UpdateRichPresence(FString Status, FString Score, bool bUseScore, bool bPlaying);
};
