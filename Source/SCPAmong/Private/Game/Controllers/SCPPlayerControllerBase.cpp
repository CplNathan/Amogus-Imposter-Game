// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Controllers/SCPPlayerControllerBase.h"
#include "Blueprint/UserWidget.h"

void ASCPPlayerControllerBase::SetActiveWidget(FWidgetOption Widget)
{
	if (!IsLocalPlayerController())
		return;

	if (ActiveWidget->IsValidLowLevel())
	{
		if (ActiveWidget->IsInViewport() && ActiveWidget->StaticClass() == Widget.Widget)
			return;
		else
			ActiveWidget->RemoveFromViewport();
	}

	if (!Widget.Widget)
		return;

	ActiveWidget = CreateWidget<UUserWidget>(this, Widget.Widget); // Create Widget

	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport(); // Add it to the viewport so the Construct() method in the UUserWidget:: is run.

		if (Widget.bLockToUI)
		{
			bShowMouseCursor = true;

			SetInputMode(FInputModeUIOnly());
		}
		else
		{
			bShowMouseCursor = false;

			SetInputMode(FInputModeGameOnly());
		}
	}
}