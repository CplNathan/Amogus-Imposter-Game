// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/Controllers/SCPPlayerControllerBase.h"
#include "SCPMainMenuPlayerController.generated.h"

class ULevelStreamingDynamic;

/**
 * 
 */
UCLASS()
class SCPAMONG_API ASCPMainMenuPlayerController : public ASCPPlayerControllerBase
{
	GENERATED_BODY()

private:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FWidgetOption MainMenuWidgetClass;

public:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
		TSoftObjectPtr<UWorld> BackgroundWorld;

	UPROPERTY()
		ULevelStreamingDynamic* LoadedBackground;

	UFUNCTION()
		void OnBackgroundVisible();
};
