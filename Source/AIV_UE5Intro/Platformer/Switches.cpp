#include "Switches.h"

#include "Activable.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------
// ASwitchBase
// ---------------------------------------------------------------------

ASwitchBase::ASwitchBase()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Handle"));
	Handle->SetMobility(EComponentMobility::Movable);
}

void ASwitchBase::SetSwitchOn(bool bOn)
{
	if (bIsOn == bOn)
	{
		return;
	}
	bIsOn = bOn;

	// Primo metodo: un ciclo sull'array di attivabili.
	for (AActivable* Target : Targets)
	{
		if (Target)
		{
			Target->SetActive(bIsOn);
		}
	}
	// Secondo metodo: Call dell'event dispatcher; risponde chi ha fatto Bind Event.
	OnSwitchChanged.Broadcast(bIsOn);

	if (ClickSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ClickSound, GetActorLocation());
	}
	UMaterialInterface* Mat = bIsOn ? OnMaterial : OffMaterial;
	if (Mat)
	{
		Handle->SetMaterial(0, Mat);
	}
	OnSwitchStateChanged();
}

// ---------------------------------------------------------------------
// APressurePlate
// ---------------------------------------------------------------------

APressurePlate::APressurePlate()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Frame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	Frame->SetupAttachment(Root);
	Frame->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.08f));
	Frame->SetRelativeLocation(FVector(0.f, 0.f, 4.f));

	Handle->SetupAttachment(Root);
	Handle->SetRelativeScale3D(FVector(1.3f, 1.3f, 0.1f));
	Handle->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
	Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // ci si cammina sopra la cornice

	if (Cylinder.Succeeded())
	{
		Frame->SetStaticMesh(Cylinder.Object);
		Handle->SetStaticMesh(Cylinder.Object);
	}

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetBoxExtent(FVector(65.f, 65.f, 40.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
}

void APressurePlate::BeginPlay()
{
	Super::BeginPlay();
	HandleRestZ = Handle->GetRelativeLocation().Z;
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &APressurePlate::HandleBeginOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &APressurePlate::HandleEndOverlap);
	if (OffMaterial)
	{
		Handle->SetMaterial(0, OffMaterial);
	}
}

void APressurePlate::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<ACharacter>(OtherActor))
	{
		SetSwitchOn(true);
	}
}

void APressurePlate::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!bStayPressed && Cast<ACharacter>(OtherActor))
	{
		SetSwitchOn(false);
	}
}

void APressurePlate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// la timeline del pulsante: la pedana scende in PressTime secondi
	PressAlpha = FMath::FInterpConstantTo(PressAlpha, bIsOn ? 1.f : 0.f, DeltaSeconds, 1.f / PressTime);
	FVector Loc = Handle->GetRelativeLocation();
	Loc.Z = HandleRestZ - PressDepth * PressAlpha;
	Handle->SetRelativeLocation(Loc);
}

// ---------------------------------------------------------------------
// AInteractLever
// ---------------------------------------------------------------------

AInteractLever::AInteractLever()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(Root);
	Base->SetRelativeScale3D(FVector(0.5f, 0.7f, 0.9f));
	Base->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	// la trace di interazione (UE12) deve "vedere" la leva
	Base->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);
	if (Cube.Succeeded())
	{
		Base->SetStaticMesh(Cube.Object);
	}

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(Root);
	Pivot->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	Pivot->SetMobility(EComponentMobility::Movable);
	Pivot->SetRelativeRotation(FRotator(-35.f, 0.f, 0.f));

	Handle->SetupAttachment(Pivot);
	Handle->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.7f));
	Handle->SetRelativeLocation(FVector(0.f, 0.f, 35.f));
	Handle->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);
	if (Cylinder.Succeeded())
	{
		Handle->SetStaticMesh(Cylinder.Object);
	}

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	LabelText->SetupAttachment(Root);
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, 190.f));
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetWorldSize(30.f);
}

void AInteractLever::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LabelText->SetText(Label);
	LabelText->SetVisibility(!Label.IsEmpty());
}

void AInteractLever::Interact_Implementation(ACharacter* ByCharacter)
{
	if (!bToggle && bIsOn)
	{
		return;
	}
	SetSwitchOn(!bIsOn);
}

FText AInteractLever::GetInteractPrompt_Implementation() const
{
	return Prompt;
}

void AInteractLever::OnSwitchStateChanged()
{
	Pivot->SetRelativeRotation(FRotator(bIsOn ? 35.f : -35.f, 0.f, 0.f));
}
