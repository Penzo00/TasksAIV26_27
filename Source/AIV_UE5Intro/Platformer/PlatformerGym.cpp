#include "PlatformerGym.h"

#include "PlatformerRunSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// la scritta sopra gli oggetti: rivolta verso -X (da dove arriva il giocatore)
	UTextRenderComponent* MakeLabel(AActor* Owner, USceneComponent* Parent)
	{
		UTextRenderComponent* Text = Owner->CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
		Text->SetupAttachment(Parent);
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextBottom);
		Text->SetWorldSize(40.f);
		Text->SetTextRenderColor(FColor(255, 230, 120));
		Text->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Text;
	}

	void PlaceLabel(UTextRenderComponent* Text, const FText& Label, float TopZ)
	{
		if (!Text)
		{
			return;
		}
		Text->SetText(Label);
		Text->SetVisibility(!Label.IsEmpty());
		// la scritta non deve scalare con la mesh: posizione assoluta sopra la cima
		Text->SetUsingAbsoluteScale(true);
		Text->SetRelativeLocation(FVector(0.f, 0.f, TopZ + 40.f));
	}
}

// ---------------------------------------------------------------------
// ASpawnedCrate
// ---------------------------------------------------------------------

ASpawnedCrate::ASpawnedCrate()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetRelativeScale3D(FVector(0.8f));
	// le casse spariscono: non sono mai un "posto sicuro" per le vite
	Tags.Add(FName(TEXT("NoSafeSpot")));
}

void ASpawnedCrate::SetCrate(float Size, UMaterialInterface* Material)
{
	Mesh->SetWorldScale3D(FVector(Size / 100.f));
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}
}

// ---------------------------------------------------------------------
// AScaleByDistanceActor
// ---------------------------------------------------------------------

AScaleByDistanceActor::AScaleByDistanceActor()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	if (Cube.Succeeded())
	{
		ShapeMesh = Cube.Object;
	}
	LabelText = MakeLabel(this, Root);
}

void AScaleByDistanceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySettings();
}

void AScaleByDistanceActor::ApplySettings()
{
	if (ShapeMesh)
	{
		Mesh->SetStaticMesh(ShapeMesh);
	}
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}
	CurrentScale = MaxScale;
	SetScaleNow(CurrentScale);
}

void AScaleByDistanceActor::SetScaleNow(float Scale)
{
	const FVector Factor(Scale, Scale, bScaleHeight ? Scale : 1.f);
	Mesh->SetRelativeScale3D(BaseSize / 100.f * Factor);
	PlaceLabel(LabelText, Label, BaseSize.Z * Factor.Z * 0.5f);
}

void AScaleByDistanceActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}
	// distanza -> 0..1 -> scala (Map Range Clamped in Blueprint)
	const float Distance = FVector::Dist(Player->GetActorLocation(), GetActorLocation());
	const float Alpha = FMath::Clamp((Distance - NearDistance) / FMath::Max(1.f, FarDistance - NearDistance), 0.f, 1.f);
	const float Wanted = FMath::Lerp(MinScale, MaxScale, Alpha);
	CurrentScale = InterpSpeed > 0.f ? FMath::FInterpTo(CurrentScale, Wanted, DeltaSeconds, InterpSpeed) : Wanted;
	SetScaleNow(CurrentScale);
}

// ---------------------------------------------------------------------
// AModelCyclerActor
// ---------------------------------------------------------------------

AModelCyclerActor::AModelCyclerActor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	UStaticMesh* Shapes[] = { Cube.Object, Cylinder.Object, Sphere.Object, Cone.Object };
	for (UStaticMesh* M : Shapes)
	{
		if (M)
		{
			Models.Add(M);
		}
	}
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	LabelText = MakeLabel(this, Root);
}

void AModelCyclerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySettings();
}

void AModelCyclerActor::ApplySettings()
{
	Mesh->SetRelativeScale3D(Size / 100.f);
	Mesh->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
	ShowModel(0);
	PlaceLabel(LabelText, Label, Size.Z * 0.5f);
}

void AModelCyclerActor::BeginPlay()
{
	Super::BeginPlay();
	ShowModel(0);
	// Set Timer by Event, in loop: ogni Interval secondi il modello dopo
	GetWorldTimerManager().SetTimer(CycleHandle, this, &AModelCyclerActor::NextModel, Interval, true);
}

void AModelCyclerActor::NextModel()
{
	if (Models.Num() > 0)
	{
		ShowModel((CurrentIndex + 1) % Models.Num());
	}
}

void AModelCyclerActor::ShowModel(int32 Index)
{
	if (!Models.IsValidIndex(Index))
	{
		return;
	}
	CurrentIndex = Index;
	Mesh->SetStaticMesh(Models[Index]);
	if (Materials.Num() > 0)
	{
		UMaterialInterface* Mat = Materials.IsValidIndex(Index) ? Materials[Index] : Materials[0];
		if (Mat)
		{
			Mesh->SetMaterial(0, Mat);
		}
	}
}

// ---------------------------------------------------------------------
// ARandomPresenceActor
// ---------------------------------------------------------------------

ARandomPresenceActor::ARandomPresenceActor()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	if (Cube.Succeeded())
	{
		ShapeMesh = Cube.Object;
	}
	LabelText = MakeLabel(this, Root);
}

void ARandomPresenceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySettings();
}

void ARandomPresenceActor::ApplySettings()
{
	if (ShapeMesh)
	{
		Mesh->SetStaticMesh(ShapeMesh);
	}
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}
	Mesh->SetRelativeScale3D(Size / 100.f);
	PlaceLabel(LabelText, Label, Size.Z * 0.5f);
}

void ARandomPresenceActor::BeginPlay()
{
	Super::BeginPlay();
	// Random Bool with Weight (0.35): se esce falso, l'oggetto non c'è mai stato
	const bool bPresent = FMath::FRand() < SpawnChance;
	UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	if (bAnnounce && Run && GetWorld())
	{
		// il messaggio parte dopo un attimo, quando l'HUD è pronto ad ascoltare
		const FString Text = bPresent ? PresentMessage : AbsentMessage;
		const FLinearColor Color = bPresent ? FLinearColor(0.5f, 1.f, 0.5f) : FLinearColor(0.8f, 0.8f, 0.8f);
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(Run, [Run, Text, Color]()
		{
			Run->PushMessage(Text, Color);
		}), 1.f, false);
	}
	if (!bPresent)
	{
		Destroy();
	}
}
