#include "Platformer/PlatformerZone.h"

#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APlatformerZone::APlatformerZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);
	RootComponent = Trigger;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Trigger);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCastShadow(false);
	Visual->SetGenerateOverlapEvents(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Visual->SetStaticMesh(Cube.Object);
	}

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	LabelText->SetupAttachment(Trigger);
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextBottom);
	LabelText->SetWorldSize(40.f);
	LabelText->SetTextRenderColor(FColor::White);
}

void APlatformerZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyZoneSettings();
}

void APlatformerZone::ApplyZoneSettings()
{
	Trigger->SetBoxExtent(ZoneExtent);

	// La parte visibile è una "pedana" sottile sul fondo della zona, non tutta la
	// scatola: così non nasconde il personaggio quando ci entra.
	// Il cubo dell'engine è largo 100 cm: scala = estensione * 2 / 100.
	Visual->SetRelativeScale3D(FVector(ZoneExtent.X / 50.f, ZoneExtent.Y / 50.f, VisualThickness / 100.f));
	Visual->SetRelativeLocation(FVector(0.f, 0.f, -ZoneExtent.Z + VisualThickness * 0.5f));
	Visual->SetVisibility(bShowVisual);
	if (VisualMaterial)
	{
		Visual->SetMaterial(0, VisualMaterial);
	}

	LabelText->SetText(Label);
	LabelText->SetVisibility(!Label.IsEmpty());
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, ZoneExtent.Z + 20.f));
}

void APlatformerZone::BeginPlay()
{
	Super::BeginPlay();
	ApplyZoneSettings();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &APlatformerZone::HandleBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &APlatformerZone::HandleEndOverlap);
}

bool APlatformerZone::IsValidTarget(AActor* Other) const
{
	const ACharacter* Character = Cast<ACharacter>(Other);
	if (!Character)
	{
		return false;
	}
	return !bOnlyPlayer || Character->IsPlayerControlled();
}

void APlatformerZone::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsValidTarget(OtherActor))
	{
		OnCharacterEntered(Cast<ACharacter>(OtherActor));
	}
}

void APlatformerZone::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsValidTarget(OtherActor))
	{
		OnCharacterExited(Cast<ACharacter>(OtherActor));
	}
}
