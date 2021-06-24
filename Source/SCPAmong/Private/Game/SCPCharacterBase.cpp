// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SCPCharacterBase.h"
#include "Game/Controllers/SCPPlayerController.h"
#include "Game/Components/FogOfWarCullingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASCPCharacterBase::ASCPCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	FogComponent = CreateDefaultSubobject<UFogOfWarCullingComponent>("Fog Component");

	SelectedMesh = CreateDefaultSubobject<UStaticMeshComponent>("Selected Indicator");
	SelectedMesh->SetHiddenInGame(true);
	SelectedMesh->SetupAttachment(RootComponent);
	SelectedMesh->SetUsingAbsoluteRotation(true);

	bUseControllerRotationYaw = false;

	bReported = false;
	bDead = false;
	bDeadIdle = false;
	bIsImposter = false;
}

void ASCPCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCPCharacterBase, bReported);
	DOREPLIFETIME(ASCPCharacterBase, Colour);
	DOREPLIFETIME(ASCPCharacterBase, bDead);
	DOREPLIFETIME_CONDITION(ASCPCharacterBase, bIsImposter, COND_OwnerOnly);
	// store the player colour and imposter status in a replicated array in the character set by the gamestate
}

void ASCPCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	SelectedMesh->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));

	CharacterColourInstance = UMaterialInstanceDynamic::Create(CharacterColourMaterial, this);
	GetMesh()->SetMaterial(CharacterColourMaterialIndex, CharacterColourInstance);
	GetMesh()->bRenderCustomDepth = true;
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

// Called when the game starts or when spawned
void ASCPCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (bDead)
		bDeadIdle = true;

	OnVisualUpdate();
}

void ASCPCharacterBase::OnVisualUpdate()
{
	// For client playing as listen server
	ASCPCharacterBase* LocalCharacter = GetWorld()->GetFirstPlayerController()->GetPawn<ASCPCharacterBase>();
	if (LocalCharacter)
	{
		if (LocalCharacter->bIsImposter)
			GetMesh()->CustomDepthStencilValue = (bIsImposter && !IsLocallyControlled() ? CD_ImposterOutline : CD_AlwaysVisibile);
	}
	CharacterColourInstance->SetVectorParameterValue(CharacterColourParameter, Colour.ToFColor(true));
}

// Called every frame
void ASCPCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ASCPPlayerController* LocalController = Cast<ASCPPlayerController>(GetWorld()->GetFirstPlayerController());

	APawn* ControlledLocalPawn = LocalController->GetPawn();
	SetActorHiddenInGame(!IsNetRelevantFor(LocalController, ControlledLocalPawn, FVector::ZeroVector));

	bool bIsSelected = LocalController->GetSelectedActor() == Cast<APawn>(this);
	SelectedMesh->SetHiddenInGame(!bIsSelected);
}

// Called to bind functionality to input
void ASCPCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("Forward", this, &ASCPCharacterBase::MoveForward);
	PlayerInputComponent->BindAxis("Left", this, &ASCPCharacterBase::MoveLeft);
}

void ASCPCharacterBase::MoveForward(float Delta)
{
	AddMovementInput(FVector(1, 0, 0), Delta);
}

void ASCPCharacterBase::MoveLeft(float Delta)
{
	AddMovementInput(FVector(0, 1, 0), Delta);
}

bool ASCPCharacterBase::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	if (bAlwaysRelevant || IsOwnedBy(ViewTarget) || IsOwnedBy(RealViewer) || this == ViewTarget || ViewTarget == GetInstigator())
	{
		return true;
	}

	if (Cast<AController>(RealViewer)->GetStateName() == NAME_Spectating)
		return true; // stream for spectators

	if (!ViewTarget)
		return false;

	if (FVector::Dist2D(GetActorLocation(), ViewTarget->GetActorLocation()) > FogComponent->ArcRadius)
	{
		return false;
	}
	else
	{
		FHitResult CurrentHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		bool bHit = GetWorld()->LineTraceSingleByChannel(CurrentHit, GetActorLocation(), ViewTarget->GetActorLocation(), ECollisionChannel::ECC_FogTrace, Params);

		return !bHit || CurrentHit.Actor == ViewTarget;
	}

	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}