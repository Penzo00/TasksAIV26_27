#include "Windmill.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AWindmill::AWindmill()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Tower = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tower"));
	Tower->SetupAttachment(Root);
	if (Cylinder.Succeeded())
	{
		Tower->SetStaticMesh(Cylinder.Object);
	}

	Head = CreateDefaultSubobject<USceneComponent>(TEXT("Head"));
	Head->SetupAttachment(Root);
	Head->SetMobility(EComponentMobility::Movable);

	Hub = CreateDefaultSubobject<USceneComponent>(TEXT("Hub"));
	Hub->SetupAttachment(Head);
	Hub->SetMobility(EComponentMobility::Movable);

	HubMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HubMesh"));
	HubMesh->SetupAttachment(Hub);
	HubMesh->SetMobility(EComponentMobility::Movable);
	if (Cylinder.Succeeded())
	{
		HubMesh->SetStaticMesh(Cylinder.Object);
	}

	// quattro pale figlie del mozzo: girando il mozzo girano tutte
	for (int32 i = 0; i < 4; ++i)
	{
		UStaticMeshComponent* Blade = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Blade%d"), i));
		Blade->SetupAttachment(Hub);
		Blade->SetMobility(EComponentMobility::Movable);
		if (Cube.Succeeded())
		{
			Blade->SetStaticMesh(Cube.Object);
		}
		Blades.Add(Blade);
	}
}

void AWindmill::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyWindmillSettings();
}

void AWindmill::ApplyWindmillSettings()
{
	Tower->SetRelativeScale3D(FVector(1.6f, 1.6f, TowerHeight / 100.f));
	Tower->SetRelativeLocation(FVector(0.f, 0.f, TowerHeight * 0.5f));
	Head->SetRelativeLocation(FVector(0.f, 0.f, TowerHeight));

	// il mozzo sporge in avanti (X); il suo cilindro è coricato lungo X (Pitch -90)
	Hub->SetRelativeLocation(FVector(110.f, 0.f, 0.f));
	HubMesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	HubMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.6f));

	// le pale sono "raggi" nel piano YZ del mozzo, ruotate di 90° l'una dall'altra (roll)
	for (int32 i = 0; i < Blades.Num(); ++i)
	{
		UStaticMeshComponent* Blade = Blades[i];
		const FRotator Spoke(0.f, 0.f, 90.f * i);
		Blade->SetRelativeRotation(Spoke);
		Blade->SetRelativeLocation(Spoke.RotateVector(FVector(0.f, 0.f, BladeLength * 0.5f + 40.f)));
		Blade->SetRelativeScale3D(FVector(0.15f, 0.7f, BladeLength / 100.f));
		Blade->SetCollisionEnabled(bBladesCollide ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (BladeMaterial)
		{
			Blade->SetMaterial(0, BladeMaterial);
		}
	}
	if (TowerMaterial)
	{
		Tower->SetMaterial(0, TowerMaterial);
		HubMesh->SetMaterial(0, TowerMaterial);
	}
}

void AWindmill::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Le pale: si somma una piccola rotazione a ogni frame (Combine Rotators),
	// moltiplicata per il Delta Time.
	Hub->AddLocalRotation(FRotator(0.f, 0.f, BladeSpeed * DeltaSeconds));

	if (!bLookAtPlayer)
	{
		return;
	}
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	// Find Look at Rotation, portata nello spazio dell'actor, poi il Clamp
	const FRotator Look = UKismetMathLibrary::FindLookAtRotation(Head->GetComponentLocation(), Player->GetActorLocation());
	const float LocalYaw = FRotator::NormalizeAxis(Look.Yaw - GetActorRotation().Yaw);
	const float Clamped = FMath::Clamp(LocalYaw, -YawLimit, YawLimit);
	const float CurrentYaw = Head->GetRelativeRotation().Yaw;
	Head->SetRelativeRotation(FRotator(0.f, FMath::FInterpTo(CurrentYaw, Clamped, DeltaSeconds, 2.f), 0.f));
}
