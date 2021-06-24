// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Controllers/SCPMainMenuPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Camera/CameraActor.h"

void ASCPMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

    SetActiveWidget(MainMenuWidgetClass);

    bool bSuccess = false;
    LoadedBackground = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(GetWorld(), BackgroundWorld, FVector::ZeroVector, FRotator::ZeroRotator, bSuccess);

    if (LoadedBackground && bSuccess)
    {
        LoadedBackground->OnLevelShown.AddDynamic(this, &ASCPMainMenuPlayerController::OnBackgroundVisible);

        LoadedBackground->SetShouldBeLoaded(true);
        LoadedBackground->SetShouldBeVisible(true);
    }
}

void ASCPMainMenuPlayerController::OnBackgroundVisible()
{
    ULevel* level = LoadedBackground->GetLoadedLevel();
    if (!level)
        return;

    for (AActor* Actor : level->Actors)
    {
        if (!Actor->IsA(ACameraActor::StaticClass()))
            continue;

        if (Actor->ActorHasTag("MainMenu"))
        {
            SetViewTarget(Actor);
        }
    }
}

void ASCPMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    SetActiveWidget(FWidgetOption());
}