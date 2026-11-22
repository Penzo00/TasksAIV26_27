#include "Platformer/SpinningObstacle.h"

#include "Platformer/PlatformerRunSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

ASpinningObstacle::ASpinningObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	RootComponent = Pivot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(Pivot);
	if (Cylinder.Succeeded())
	{
		Base->SetStaticMesh(Cylinder.Object);
	}

	// La sbarra non blocca: sovrappone il personaggio e lo spinge via con LaunchCharacter.
	// Così non lo "incastra" contro i muri e il movimento resta prevedibile.
	Bar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bar"));
	Bar->SetupAttachment(Pivot);
	Bar->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Bar->SetGenerateOverlapEvents(true);
	if (Cube.Succeeded())
	{
		Bar->SetStaticMesh(Cube.Object);
	}
}

void ASpinningObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyBarSize();
}

void ASpinningObstacle::ApplyBarSize()
{
	Bar->SetRelativeScale3D(BarSize / 100.f);
	Bar->SetRelativeLocation(FVector(0.f, 0.f, BarHeight));
	// il cilindro dell'engine è alto 100 cm con il pivot al centro
	Base->SetRelativeScale3D(FVector(0.4f, 0.4f, BarHeight / 100.f));
	Base->SetRelativeLocation(FVector(0.f, 0.f, BarHeight * 0.5f));
}

void ASpinningObstacle::BeginPlay()
{
	Super::BeginPlay();
	ApplyBarSize();
	Bar->OnComponentBeginOverlap.AddDynamic(this, &ASpinningObstacle::HandleBarOverlap);
}

void ASpinningObstacle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Bar->AddLocalRotation(FRotator(0.f, DegreesPerSecond * DeltaSeconds, 0.f));
}

void ASpinningObstacle::HandleBarOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Character->IsPlayerControlled())
	{
		return;
	}
	if (bKillOnTouch)
	{
		if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
		{
			Run->KillPlayerAndRestart(0.3f);
		}
		return;
	}
	// Spinta perpendicolare alla sbarra, nel verso in cui sta girando, più un saltello.
	FVector Away = Character->GetActorLocation() - GetActorLocation();
	Away.Z = 0.f;
	const FVector Tangent = FVector::CrossProduct(FVector::UpVector, Away.GetSafeNormal())
		* FMath::Sign(DegreesPerSecond);
	const FVector Launch = (Tangent + Away.GetSafeNormal() * 0.5f).GetSafeNormal() * PushStrength
		+ FVector(0.f, 0.f, PushStrength * 0.4f);
	Character->LaunchCharacter(Launch, true, true);
}
