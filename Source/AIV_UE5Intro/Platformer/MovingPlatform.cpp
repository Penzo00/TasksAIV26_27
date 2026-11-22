#include "Platformer/MovingPlatform.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMovingPlatform::AMovingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetMobility(EComponentMobility::Movable);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
}

void AMovingPlatform::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPlatformSize();
}

void AMovingPlatform::ApplyPlatformSize()
{
	Mesh->SetWorldScale3D(PlatformSize / 100.f);
}

void AMovingPlatform::BeginPlay()
{
	Super::BeginPlay();
	StartLocation = GetActorLocation();
	Clock = -StartDelay;
}

void AMovingPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	if (Clock < 0.f)
	{
		return;
	}

	// Un ciclo completo: andata, sosta, ritorno, sosta.
	const float Cycle = 2.f * (TravelTime + PauseTime);
	const float T = FMath::Fmod(Clock, Cycle);
	float Alpha;
	if (T < TravelTime)
	{
		Alpha = T / TravelTime;                                   // andata
	}
	else if (T < TravelTime + PauseTime)
	{
		Alpha = 1.f;                                              // sosta all'arrivo
	}
	else if (T < 2.f * TravelTime + PauseTime)
	{
		Alpha = 1.f - (T - TravelTime - PauseTime) / TravelTime;  // ritorno
	}
	else
	{
		Alpha = 0.f;                                              // sosta alla partenza
	}
	if (bSmooth)
	{
		Alpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	}

	// EndOffset è nello spazio locale dell'actor: se si ruota la piattaforma,
	// la traiettoria ruota con lei.
	const FVector End = StartLocation + GetActorRotation().RotateVector(EndOffset);
	SetActorLocation(FMath::Lerp(StartLocation, End, Alpha));
}
