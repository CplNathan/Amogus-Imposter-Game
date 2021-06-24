// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SCPPlayerControllerBase.generated.h"

USTRUCT(BlueprintType)
struct FWidgetOption
{
	GENERATED_BODY()

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TSubclassOf<class UUserWidget> Widget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool bLockToUI;

	FWidgetOption()
	{
		bLockToUI = false;
	}
};

/**
 * 
 */
UCLASS()
class SCPAMONG_API ASCPPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION()
		void SetActiveWidget(FWidgetOption Widget);

	UPROPERTY()
		class UUserWidget* ActiveWidget;
};
