#include "Activable.h"

#include "Switches.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------
// AActivable (il padre)
// ---------------------------------------------------------------------

AActivable::AActivable()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;
}

void AActivable::BeginPlay()
{
	Super::BeginPlay();
	bActive = bStartActive;
	Progress = bActive ? 1.f : 0.f;
	ApplyAlpha(Progress);

	// Secondo metodo della lezione: Bind Event all'event dispatcher dell'interruttore.
	for (ASwitchBase* Switch : ListenTo)
	{
		if (Switch)
		{
			Switch->OnSwitchChanged.AddDynamic(this, &AActivable::HandleSwitchChanged);
		}
	}
}

void AActivable::HandleSwitchChanged(bool bOn)
{
	SetActive(bOn);
}

void AActivable::SetActive(bool bNewActive)
{
	if (bActive == bNewActive)
	{
		return;
	}
	bActive = bNewActive;
	if (ActivateSound && bActive)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ActivateSound, GetActorLocation());
	}
	OnActivationChanged(bActive);
}

void AActivable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Target = bActive ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(Progress, Target))
	{
		return;
	}
	// È la timeline della lezione: Progress va da 0 a 1 in TransitionTime
	// secondi (Play) o torna indietro (Reverse).
	Progress = FMath::FInterpConstantTo(Progress, Target, DeltaSeconds, 1.f / TransitionTime);
	float Alpha = Progress;
	if (TransitionCurve)
	{
		Alpha = TransitionCurve->GetFloatValue(Progress);
	}
	else
	{
		Alpha = FMath::InterpEaseInOut(0.f, 1.f, Progress, 2.f);
	}
	ApplyAlpha(Alpha);
}

// ---------------------------------------------------------------------
// AActivableDoor
// ---------------------------------------------------------------------

AActivableDoor::AActivableDoor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	LeftHinge = CreateDefaultSubobject<USceneComponent>(TEXT("LeftHinge"));
	LeftHinge->SetupAttachment(Root);
	RightHinge = CreateDefaultSubobject<USceneComponent>(TEXT("RightHinge"));
	RightHinge->SetupAttachment(Root);

	LeftPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPanel"));
	LeftPanel->SetupAttachment(LeftHinge);
	RightPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPanel"));
	RightPanel->SetupAttachment(RightHinge);
	for (UStaticMeshComponent* Panel : { LeftPanel.Get(), RightPanel.Get() })
	{
		Panel->SetMobility(EComponentMobility::Movable);
		if (Cube.Succeeded())
		{
			Panel->SetStaticMesh(Cube.Object);
		}
	}
	LeftHinge->SetMobility(EComponentMobility::Movable);
	RightHinge->SetMobility(EComponentMobility::Movable);
}

void AActivableDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDoorSize();
}

void AActivableDoor::ApplyDoorSize()
{
	const float Half = DoorWidth * 0.5f;
	// Cardini ai due lati del vano; ogni anta è larga metà del vano.
	LeftHinge->SetRelativeLocation(FVector(0.f, -Half, 0.f));
	RightHinge->SetRelativeLocation(FVector(0.f, Half, 0.f));
	const FVector Scale(DoorThickness / 100.f, Half / 100.f, DoorHeight / 100.f);
	LeftPanel->SetRelativeScale3D(Scale);
	RightPanel->SetRelativeScale3D(Scale);
	LeftPanel->SetRelativeLocation(FVector(0.f, Half * 0.5f, DoorHeight * 0.5f));
	RightPanel->SetRelativeLocation(FVector(0.f, -Half * 0.5f, DoorHeight * 0.5f));
	if (DoorMaterial)
	{
		LeftPanel->SetMaterial(0, DoorMaterial);
		RightPanel->SetMaterial(0, DoorMaterial);
	}
	ApplyAlpha(bStartActive ? 1.f : 0.f);
}

void AActivableDoor::ApplyAlpha(float Alpha)
{
	const float Half = DoorWidth * 0.5f;
	if (bSliding)
	{
		LeftHinge->SetRelativeLocationAndRotation(FVector(0.f, -Half - Half * Alpha, 0.f), FRotator::ZeroRotator);
		RightHinge->SetRelativeLocationAndRotation(FVector(0.f, Half + Half * Alpha, 0.f), FRotator::ZeroRotator);
	}
	else
	{
		LeftHinge->SetRelativeLocationAndRotation(FVector(0.f, -Half, 0.f), FRotator(0.f, -OpenAngle * Alpha, 0.f));
		RightHinge->SetRelativeLocationAndRotation(FVector(0.f, Half, 0.f), FRotator(0.f, OpenAngle * Alpha, 0.f));
	}
}

// ---------------------------------------------------------------------
// AActivableLight
// ---------------------------------------------------------------------

AActivableLight::AActivableLight()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(40.f);
	Light->SetAttenuationRadius(900.f);
	Light->SetLightColor(FLinearColor(1.f, 0.78f, 0.5f));

	Bulb = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bulb"));
	Bulb->SetupAttachment(Root);
	Bulb->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bulb->SetCastShadow(false);   // il bulbo non deve fare ombra alla sua luce
	Bulb->SetRelativeScale3D(FVector(0.2f));
	if (Sphere.Succeeded())
	{
		Bulb->SetStaticMesh(Sphere.Object);
	}
}

void AActivableLight::BeginPlay()
{
	// "Salvare l'intensità" (UE7): la si legge PRIMA che il padre chiami ApplyAlpha.
	SavedIntensity = Light->Intensity;
	Bulb->SetVisibility(bShowBulb);
	Super::BeginPlay();
}

void AActivableLight::ApplyAlpha(float Alpha)
{
	Light->SetIntensity(SavedIntensity * Alpha);
}

// ---------------------------------------------------------------------
// AActivableMover
// ---------------------------------------------------------------------

AActivableMover::AActivableMover()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
	TransitionTime = 2.5f;
}

void AActivableMover::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPlatformSize();
}

void AActivableMover::ApplyPlatformSize()
{
	Mesh->SetRelativeScale3D(PlatformSize / 100.f);
	ApplyAlpha(bStartActive ? 1.f : 0.f);
}

void AActivableMover::ApplyAlpha(float Alpha)
{
	Mesh->SetRelativeLocation(EndOffset * Alpha);
}
