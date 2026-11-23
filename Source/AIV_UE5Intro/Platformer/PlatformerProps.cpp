#include "PlatformerProps.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------
// AJumpPad
// ---------------------------------------------------------------------

AJumpPad::AJumpPad()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	RootComponent = Base;
	Base->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.2f));

	Pad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pad"));
	Pad->SetupAttachment(Base);
	Pad->SetMobility(EComponentMobility::Movable);
	Pad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Pad->SetRelativeScale3D(FVector(0.85f, 0.85f, 0.4f));
	Pad->SetRelativeLocation(FVector(0.f, 0.f, 60.f));

	if (Cylinder.Succeeded())
	{
		Base->SetStaticMesh(Cylinder.Object);
		Pad->SetStaticMesh(Cylinder.Object);
	}

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Base);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	// la Base è scalata (1.4, 1.4, 0.2): l'estensione è nello spazio scalato
	Trigger->SetBoxExtent(FVector(40.f, 40.f, 150.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 250.f));
}

void AJumpPad::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (PadMaterial)
	{
		Pad->SetMaterial(0, PadMaterial);
	}
}

void AJumpPad::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AJumpPad::HandleOverlap);
}

void AJumpPad::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character)
	{
		return;
	}
	// GD12: Launch Character, con la velocità ruotata come l'actor
	const FVector Velocity = GetActorRotation().RotateVector(LaunchVelocity);
	Character->LaunchCharacter(Velocity, bOverrideHorizontal, true);
	SquashTime = 0.25f;
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BounceSound, GetActorLocation());
	}
}

void AJumpPad::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// piccolo "rimbalzo" della pedana: feedback visivo
	SquashTime = FMath::Max(0.f, SquashTime - DeltaSeconds);
	const float Squash = FMath::Sin((SquashTime / 0.25f) * PI) * 0.25f;
	Pad->SetRelativeScale3D(FVector(0.85f + Squash * 0.3f, 0.85f + Squash * 0.3f, 0.4f - Squash));
}

// ---------------------------------------------------------------------
// ACrumblingPlatform
// ---------------------------------------------------------------------

ACrumblingPlatform::ACrumblingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetMobility(EComponentMobility::Movable);
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(SceneRoot);
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
}

void ACrumblingPlatform::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPlatformSize();
}

void ACrumblingPlatform::ApplyPlatformSize()
{
	Mesh->SetRelativeScale3D(PlatformSize / 100.f);
	Mesh->SetRelativeLocation(FVector::ZeroVector);
	// il trigger è un sottile strato sopra la piattaforma: scatta quando ci si sale
	Trigger->SetBoxExtent(FVector(PlatformSize.X * 0.5f - 5.f, PlatformSize.Y * 0.5f - 5.f, 20.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, PlatformSize.Z * 0.5f + 20.f));
	if (PlatformMaterial)
	{
		Mesh->SetMaterial(0, PlatformMaterial);
	}
}

void ACrumblingPlatform::BeginPlay()
{
	Super::BeginPlay();
	ApplyPlatformSize();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ACrumblingPlatform::HandleOverlap);
}

void ACrumblingPlatform::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (State != EState::Idle || !Cast<ACharacter>(OtherActor))
	{
		return;
	}
	State = EState::Shaking;
	StateTime = 0.f;
	if (WarningMaterial)
	{
		Mesh->SetMaterial(0, WarningMaterial);
	}
}

void ACrumblingPlatform::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (State != EState::Shaking)
	{
		return;
	}
	StateTime += DeltaSeconds;
	// trema sempre di più: avvisa il giocatore che deve andarsene
	const float Strength = ShakeAmount * FMath::Clamp(StateTime / FMath::Max(ShakeTime, 0.01f), 0.2f, 1.f);
	Mesh->SetRelativeLocation(FVector(FMath::FRandRange(-Strength, Strength), FMath::FRandRange(-Strength, Strength), 0.f));
	if (StateTime >= ShakeTime)
	{
		Fall();
	}
}

void ACrumblingPlatform::Fall()
{
	State = EState::Gone;
	Mesh->SetVisibility(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (CrumbleSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CrumbleSound, GetActorLocation());
	}
	if (RespawnTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(RespawnHandle, this, &ACrumblingPlatform::Respawn, RespawnTime, false);
	}
}

void ACrumblingPlatform::Respawn()
{
	State = EState::Idle;
	Mesh->SetRelativeLocation(FVector::ZeroVector);
	Mesh->SetVisibility(true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (PlatformMaterial)
	{
		Mesh->SetMaterial(0, PlatformMaterial);
	}
	// se il personaggio è già dentro il trigger non arriva un nuovo overlap: lo controlliamo
	TArray<AActor*> Inside;
	Trigger->GetOverlappingActors(Inside, ACharacter::StaticClass());
	if (Inside.Num() > 0)
	{
		State = EState::Shaking;
		StateTime = 0.f;
	}
}
