#include "MovingPlatform.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
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
	if (PlatformMaterial)
	{
		Mesh->SetMaterial(0, PlatformMaterial);
	}
}

void AMovingPlatform::UpdateSound(int32 Phase)
{
	if (Phase == LastPhase || !MoveSound)
	{
		LastPhase = Phase;
		return;
	}
	LastPhase = Phase;
	// fasi: 0 andata, 1 sosta, 2 ritorno, 3 sosta
	if (Phase == 0 || Phase == 2)
	{
		if (!Audio)
		{
			Audio = UGameplayStatics::SpawnSoundAttached(MoveSound, Mesh, NAME_None, FVector::ZeroVector,
				EAttachLocation::KeepRelativeOffset, true, 1.f, 1.f, 0.f, nullptr, nullptr, false);
		}
		if (Audio)
		{
			Audio->SetPitchMultiplier(Phase == 0 ? PitchForward : PitchBackward);
			Audio->Play();
		}
	}
	else if (Audio)
	{
		Audio->Stop();
	}
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
	int32 Phase;
	if (T < TravelTime)
	{
		Alpha = T / TravelTime;                                   // andata
		Phase = 0;
	}
	else if (T < TravelTime + PauseTime)
	{
		Alpha = 1.f;                                              // sosta all'arrivo
		Phase = 1;
	}
	else if (T < 2.f * TravelTime + PauseTime)
	{
		Alpha = 1.f - (T - TravelTime - PauseTime) / TravelTime;  // ritorno
		Phase = 2;
	}
	else
	{
		Alpha = 0.f;                                              // sosta alla partenza
		Phase = 3;
	}
	UpdateSound(Phase);
	if (bSmooth)
	{
		Alpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	}

	// EndOffset è nello spazio locale dell'actor: se si ruota la piattaforma,
	// la traiettoria ruota con lei.
	const FVector End = StartLocation + GetActorRotation().RotateVector(EndOffset);
	SetActorLocation(FMath::Lerp(StartLocation, End, Alpha));
}
