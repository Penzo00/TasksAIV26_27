#include "CubeSpawner.h"

#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APhysicsCube::APhysicsCube()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	RootComponent = Mesh;
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
}

ACubeSpawner::ACubeSpawner()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#if WITH_EDITORONLY_DATA
	Icon = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
	if (Icon)
	{
		Icon->SetupAttachment(RootComponent);
	}
#endif
}

void ACubeSpawner::BeginPlay()
{
	Super::BeginPlay();
	SpawnCubes();
}

void ACubeSpawner::SpawnCubes()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FTransform Frame = GetActorTransform();
	const float HalfRows = (Rows - 1) * 0.5f;
	const float HalfCols = (Columns - 1) * 0.5f;
	int32 Count = 0;

	// i due For Loop: righe (X) e colonne (Y)
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Col = 0; Col < Columns; ++Col)
		{
			const FVector Local((Row - HalfRows) * Spacing, (Col - HalfCols) * Spacing, 0.f);
			const FVector Start = Frame.TransformPosition(Local + FVector(0.f, 0.f, 200.f));
			const FVector End = Start - FVector(0.f, 0.f, 3000.f);

			// il trace trova il pavimento sotto la cella
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(CubeSpawnerTrace), false, this);
			const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
			if (bDebugTraces)
			{
				DrawDebugLine(World, Start, bHit ? Hit.ImpactPoint : End, bHit ? FColor::Green : FColor::Red, false, 5.f);
			}
			if (!bHit)
			{
				continue;
			}

			for (int32 Layer = 0; Layer < Layers; ++Layer)
			{
				const FVector Location = Hit.ImpactPoint + FVector(0.f, 0.f, CubeSize * (0.5f + Layer) + 1.f);
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				APhysicsCube* NewCube = World->SpawnActor<APhysicsCube>(APhysicsCube::StaticClass(), Location,
					FRotator(0.f, GetActorRotation().Yaw, 0.f), SpawnParams);
				if (!NewCube)
				{
					continue;
				}
				UStaticMeshComponent* CubeMesh = NewCube->GetMesh();
				CubeMesh->SetWorldScale3D(FVector(CubeSize / 100.f));
				if (Materials.Num() > 0 && Materials[Count % Materials.Num()])
				{
					CubeMesh->SetMaterial(0, Materials[Count % Materials.Num()]);
				}
				if (bSimulatePhysics)
				{
					CubeMesh->SetSimulatePhysics(true);
					CubeMesh->SetMassOverrideInKg(NAME_None, CubeMass, true);
				}
				++Count;
			}
		}
	}
}
