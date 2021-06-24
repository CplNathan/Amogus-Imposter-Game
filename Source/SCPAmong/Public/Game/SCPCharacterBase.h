// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SCPAmong/SCPAmong.h"
#include "GameFramework/Character.h"
#include "SCPCharacterBase.generated.h"

class UMaterialInterface;
class UFogOfWarCullingComponent;
class UStaticMeshComponent;

UCLASS()
class SCPAMONG_API ASCPCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCPCharacterBase();

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Colour", meta = (AllowPrivateAccess = "true"))
		FName CharacterColourParameter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Colour", meta = (AllowPrivateAccess = "true"))
		int32 CharacterColourMaterialIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Colour", meta = (AllowPrivateAccess = "true"))
		UMaterialInterface* CharacterColourMaterial;

	UPROPERTY()
		UMaterialInstanceDynamic* CharacterColourInstance;

private:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
		UFogOfWarCullingComponent* FogComponent;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
		UStaticMeshComponent* SelectedMesh;

public:
	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated)
		bool bReported;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated) // maybe replusing
		bool bDead;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly)
		bool bDeadIdle; // Used for animation blueprint idle anim (this is only set if the actor has just come into view through beginplay)

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated, ReplicatedUsing = OnVisualUpdate)
		FLinearColor Colour;

	UPROPERTY(BlueprintReadOnly, VisibleDefaultsOnly, Replicated, ReplicatedUsing = OnVisualUpdate)
		bool bIsImposter;

	UFUNCTION()
		void OnVisualUpdate();

protected:
	virtual void PostInitializeComponents() override;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveForward(float Delta);

	void MoveLeft(float Delta);

	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

};
