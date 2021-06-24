// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SCPAmong/SCPAmong.h"
#include "GameFramework/Actor.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "FogOfWarCullingActor.generated.h"

class UProceduralMeshComponent;

UCLASS()
class SCPAMONG_API AFogOfWarCullingActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFogOfWarCullingActor();

protected:
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

	UPROPERTY()
		bool bShouldRun;

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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void StartRunning(AActor* Actor);

	void StopRunning();

};
