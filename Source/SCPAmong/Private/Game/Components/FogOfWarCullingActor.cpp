// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Components/FogOfWarCullingActor.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "ProceduralMeshComponent.h"

// Sets default values
AFogOfWarCullingActor::AFogOfWarCullingActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ArcRadius = 1000;
	ArcAngle = 360;
	MeshResolution = 1;

	ZMaxTrace = 500;

	MaxSpaceBetweenArcPoints = 1;
	bIsInitialized = false;
	bQueryInProgress = false;
	bShouldRun = false;

	bDebugDraw = false;

	MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>("Procedural Mesh");
	MeshComponent->SetMaterial(0, FogMaterial);
	MeshComponent->SetMaterial(1, FogMaterial);
	MeshComponent->bRenderCustomDepth = true;
	MeshComponent->bRenderInMainPass = false;
	MeshComponent->bRenderInDepthPass = false;
	MeshComponent->bVisibleInReflectionCaptures = false;
	MeshComponent->bVisibleInRayTracing = false;
	MeshComponent->SetCastShadow(false);
	MeshComponent->SetIsReplicated(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	MeshComponent->SetCustomDepthStencilValue(1);
	MeshComponent->SetUsingAbsoluteRotation(true);
	MeshComponent->SetUsingAbsoluteLocation(true);
	MeshComponent->SetEnableGravity(false);
	MeshComponent->bIgnoreRadialImpulse = true;

	ProcMeshVertices.Init(FVector(), FMath::CeilToInt(ArcAngle * MeshResolution) + 2);
	ProcMeshTriangles.Init(int32(), (ProcMeshVertices.Num() - 2) * 3);

	MaxSpaceBetweenArcPoints = (((1 / MeshResolution) * 2 * ArcRadius) * PI) / 360;
}

void AFogOfWarCullingActor::StartRunning(AActor* Actor)
{
	ACharacter* Character = Cast<ACharacter>(Actor);


	FootOffset = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	LoSPointQueryRequest = FEnvQueryRequest(LoSPointQuery, Actor);
	LoSPointQueryRequest = LoSPointQueryRequest.SetFloatParam(TEXT("ArcRadius"), ArcRadius);
	LoSPointQueryRequest = LoSPointQueryRequest.SetFloatParam(TEXT("ArcAngle"), ArcAngle);
	LoSPointQueryRequest = LoSPointQueryRequest.SetFloatParam(TEXT("SpaceBetweenPoints"), MaxSpaceBetweenArcPoints);
	LoSPointQueryRequest = LoSPointQueryRequest.SetFloatParam(TEXT("FootOffset"), -(FootOffset / 2));

	bShouldRun = true;
}

void AFogOfWarCullingActor::StopRunning()
{
	bShouldRun = false;
}

// Called every frame
void AFogOfWarCullingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShouldRun)
		RunEQS();
}

void AFogOfWarCullingActor::RunEQS()
{
	if (!bQueryInProgress)
	{
		bQueryInProgress = true;
		LoSPointQueryRequest.Execute(EEnvQueryRunMode::AllMatching, this, &AFogOfWarCullingActor::LoSPointQueryFinished);
	}
}

void AFogOfWarCullingActor::ToggleFogDebug()
{
#if WITH_EDITORONLY_DATA
	bDebugDraw = !bDebugDraw;

	if (bDebugDraw)
	{
		MeshDebugComponent = NewObject<UProceduralMeshComponent>(this);
		MeshDebugComponent->SetMaterial(0, FogMaterial);
		MeshDebugComponent->SetMaterial(1, FogMaterial);
		MeshDebugComponent->bRenderCustomDepth = false;
		MeshDebugComponent->bRenderInMainPass = true;
		MeshDebugComponent->bRenderInDepthPass = false;
		MeshDebugComponent->bVisibleInReflectionCaptures = false;
		MeshDebugComponent->bVisibleInRayTracing = false;
		MeshDebugComponent->SetCastShadow(false);
		MeshDebugComponent->SetIsReplicated(false);
		MeshDebugComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
		MeshDebugComponent->SetCustomDepthStencilValue(1);
		MeshDebugComponent->SetUsingAbsoluteRotation(true);
		MeshDebugComponent->SetUsingAbsoluteLocation(true);
		MeshDebugComponent->SetEnableGravity(false);
		MeshDebugComponent->bIgnoreRadialImpulse = true;

		MeshDebugComponent->CreateMeshSection(0, ProcMeshVertices, ProcMeshTriangles, TArray<FVector>{}, TArray<FVector2D>{}, TArray<FColor>{}, TArray<FProcMeshTangent>{}, false);
	}
	else
	{
		if (MeshDebugComponent->IsValidLowLevel())
			MeshDebugComponent->DestroyComponent();
	}
#endif
}

void AFogOfWarCullingActor::LoSPointQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (Result->IsSuccsessful())
	{
		const UWorld* World = GetWorld();
		const FVector& OwnerLocation = GetActorLocation();

		/* Build Vertex Array Data */
		FHitResult CurrentHit;
		FCollisionQueryParams Params;
		Params.MobilityType = EQueryMobilityType::Static;
		Params.bTraceComplex = false;

		ProcMeshVertices[0] = OwnerLocation - FVector(0, 0, FootOffset);

		for (int32 i = 0; i < Result->Items.Num() - 1; i++)
		{
			const FVector& Location = Result->GetItemAsLocation(i);

			bool bHitNZ = World->LineTraceSingleByChannel(CurrentHit, Location, Location - FVector(0, 0, ZMaxTrace), ECollisionChannel::ECC_FogTrace, Params);
			FVector FinalLocation = bHitNZ ? CurrentHit.Location : Location - FVector(0, 0, FootOffset);

			ProcMeshVertices[i + 1] = (FinalLocation);
		}

		if (bIsInitialized)
		{
			MeshComponent->UpdateMeshSection(0, ProcMeshVertices, TArray<FVector>{}, TArray<FVector2D>{}, TArray<FColor>{}, TArray<FProcMeshTangent>{});

#if WITH_EDITORONLY_DATA
			if (bDebugDraw)
			{
				MeshDebugComponent->UpdateMeshSection(0, ProcMeshVertices, TArray<FVector>{}, TArray<FVector2D>{}, TArray<FColor>{}, TArray<FProcMeshTangent>{});
			}
#endif
		}
		else
		{
			/* Build Triangle Array Data */
			for (int32 i = 0; i < Result->Items.Num() - 2; i++)
			{
				ProcMeshTriangles[(i * 3)] = 0;
				ProcMeshTriangles[(i * 3) + 1] = i + 1;
				ProcMeshTriangles[(i * 3) + 2] = i + 2;
			}

			TArray<int32> CapTris = {
				0,
				1,
				Result->Items.Num() - 1
			};
			ProcMeshTriangles.Append(CapTris);

			MeshComponent->CreateMeshSection(0, ProcMeshVertices, ProcMeshTriangles, TArray<FVector>{}, TArray<FVector2D>{}, TArray<FColor>{}, TArray<FProcMeshTangent>{}, false);

			bIsInitialized = true;
		}
	}

	bQueryInProgress = false;
}