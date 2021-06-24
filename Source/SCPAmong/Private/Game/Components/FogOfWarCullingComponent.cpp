// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/Components/FogOfWarCullingComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "ProceduralMeshComponent.h"

// Sets default values for this component's properties
UFogOfWarCullingComponent::UFogOfWarCullingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	ArcRadius = 1000;
	ArcAngle = 360;
	MeshResolution = 1;

	ZMaxTrace = 500;

	MaxSpaceBetweenArcPoints = 1;
	bIsInitialized = false;
	bQueryInProgress = false;

	bDebugDraw = false;
}


// Called when the game starts
void UFogOfWarCullingComponent::BeginPlay()
{
	Super::BeginPlay();

	FootOffset = Cast<ACharacter>(GetOwner())->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	MeshComponent = NewObject<UProceduralMeshComponent>(GetOwner());
	MeshComponent->SetMaterial(0, FogMaterial);
	MeshComponent->SetMaterial(1, FogMaterial);
	MeshComponent->bRenderCustomDepth = true;
	MeshComponent->bRenderInMainPass = false;
	MeshComponent->bRenderInDepthPass = false;
	MeshComponent->bVisibleInReflectionCaptures = false;
	MeshComponent->bVisibleInRayTracing = false;
	MeshComponent->SetCastShadow(false);
	MeshComponent->bOnlyOwnerSee = true;
	MeshComponent->SetIsReplicated(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	MeshComponent->SetCustomDepthStencilValue(CD_FogVisible);
	MeshComponent->SetUsingAbsoluteRotation(true);
	MeshComponent->SetUsingAbsoluteLocation(true);
	MeshComponent->SetEnableGravity(false);
	MeshComponent->bIgnoreRadialImpulse = true;
	MeshComponent->RegisterComponent();

	ProcMeshVertices.Init(FVector(), FMath::CeilToInt(ArcAngle * MeshResolution) + 2);
	ProcMeshTriangles.Init(int32(), (ProcMeshVertices.Num() - 2) * 3);

	MaxSpaceBetweenArcPoints = (((1 / MeshResolution) * 2 * ArcRadius) * PI) / 360;

	LoSPointQueryRequest = FEnvQueryRequest(LoSPointQuery, GetOwner());
	LoSPointQueryRequest = LoSPointQueryRequest.SetFloatParam(TEXT("ArcRadius"), ArcRadius);
	LoSPointQueryRequest = LoSPointQueryRequest.SetFloatParam(TEXT("ArcAngle"), ArcAngle);
	LoSPointQueryRequest = LoSPointQueryRequest.SetFloatParam(TEXT("SpaceBetweenPoints"), MaxSpaceBetweenArcPoints);
	LoSPointQueryRequest = LoSPointQueryRequest.SetFloatParam(TEXT("FootOffset"), -(FootOffset / 2));
}

void UFogOfWarCullingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Cast<APawn>(GetOwner())->IsLocallyControlled())
		RunEQS();
}

void UFogOfWarCullingComponent::RunEQS()
{
	if (!bQueryInProgress)
	{
		bQueryInProgress = true;
		LoSPointQueryRequest.Execute(EEnvQueryRunMode::AllMatching, this, &UFogOfWarCullingComponent::LoSPointQueryFinished);
	}
}

void UFogOfWarCullingComponent::ToggleFogDebug()
{
#if WITH_EDITORONLY_DATA
	bDebugDraw = !bDebugDraw;

	if (bDebugDraw)
	{
		MeshDebugComponent = NewObject<UProceduralMeshComponent>(GetOwner());
		MeshDebugComponent->SetMaterial(0, FogMaterial);
		MeshDebugComponent->SetMaterial(1, FogMaterial);
		MeshDebugComponent->bRenderCustomDepth = false;
		MeshDebugComponent->bRenderInMainPass = true;
		MeshDebugComponent->bRenderInDepthPass = false;
		MeshDebugComponent->bVisibleInReflectionCaptures = false;
		MeshDebugComponent->bVisibleInRayTracing = false;
		MeshDebugComponent->SetCastShadow(false);
		MeshDebugComponent->bOnlyOwnerSee = true;
		MeshDebugComponent->SetIsReplicated(false);
		MeshDebugComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
		MeshDebugComponent->SetCustomDepthStencilValue(CD_FogVisible);
		MeshDebugComponent->SetUsingAbsoluteRotation(true);
		MeshDebugComponent->SetUsingAbsoluteLocation(true);
		MeshDebugComponent->SetEnableGravity(false);
		MeshDebugComponent->bIgnoreRadialImpulse = true;
		MeshDebugComponent->RegisterComponent();

		MeshDebugComponent->CreateMeshSection(0, ProcMeshVertices, ProcMeshTriangles, TArray<FVector>{}, TArray<FVector2D>{}, TArray<FColor>{}, TArray<FProcMeshTangent>{}, false);
	}
	else
	{
		if (MeshDebugComponent->IsValidLowLevel())
			MeshDebugComponent->DestroyComponent();
	}
#endif
}

void UFogOfWarCullingComponent::LoSPointQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (Result->IsSuccsessful())
	{
		const UWorld* World = GetWorld();
		const FVector& OwnerLocation = GetOwner()->GetActorLocation();

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
			FVector FinalLocation = bHitNZ ? CurrentHit.Location : Location - FVector(0, 0, FootOffset/2);

			ProcMeshVertices[i + 1] = (FinalLocation + 1);
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