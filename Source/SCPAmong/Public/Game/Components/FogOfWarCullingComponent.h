// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SCPAmong/SCPAmong.h"
#include "Components/ActorComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "FogOfWarCullingComponent.generated.h"

class UProceduralMeshComponent;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SCPAMONG_API UFogOfWarCullingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFogOfWarCullingComponent();

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Config|Required")
		UEnvQuery* LoSPointQuery;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Config|Required")
		UMaterialInterface* FogMaterial;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Config|Options")
		float ArcRadius;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Config|Options")
		float ArcAngle;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Config|Options")
		float MeshResolution;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Config|Options|Technical")
		float ZMaxTrace;

private:
	UPROPERTY()
		TArray<FVector> ProcMeshVertices;

	UPROPERTY()
		TArray<int32> ProcMeshTriangles;

	UPROPERTY()
		float MaxSpaceBetweenArcPoints;

	UPROPERTY()
		bool bIsInitialized;

	UPROPERTY()
		bool bQueryInProgress;

	UPROPERTY()
		bool bDebugDraw;

	UPROPERTY()
		float FootOffset;

protected:
	UPROPERTY()
		UProceduralMeshComponent* MeshComponent;

	UPROPERTY()
		UProceduralMeshComponent* MeshDebugComponent;

	UPROPERTY()
		FEnvQueryRequest LoSPointQueryRequest;

	void LoSPointQueryFinished(TSharedPtr<FEnvQueryResult> Result);

protected:
	UFUNCTION()
		void RunEQS();

	UFUNCTION(Exec, BlueprintCallable, CallInEditor)
		void ToggleFogDebug();

	// Called when the game starts
	virtual void BeginPlay() override;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
};
