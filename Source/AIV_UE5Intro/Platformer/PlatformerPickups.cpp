#include "PlatformerPickups.h"

#include "PlatformerHealthComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------
// APlatformerPickup (il padre)
// ---------------------------------------------------------------------

APlatformerPickup::APlatformerPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetCollisionProfileName(TEXT("Trigger"));
	Sphere->SetSphereRadius(PickupRadius);
	RootComponent = Sphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Sphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetMobility(EComponentMobility::Movable);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Sphere);
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetAttenuationRadius(250.f);
	Glow->SetCastShadows(false);   // luce di "rinforzo": niente ombre, costa poco

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	LabelText->SetupAttachment(Sphere);
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetWorldSize(28.f);
}

void APlatformerPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyPickupSettings();
}

void APlatformerPickup::ApplyPickupSettings()
{
	Sphere->SetSphereRadius(PickupRadius);
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}
	Glow->SetIntensity(GlowIntensity);
	Glow->SetLightColor(GlowColor);
	Glow->SetVisibility(GlowIntensity > 0.f);
	LabelText->SetText(Label);
	LabelText->SetVisibility(!Label.IsEmpty());
}

void APlatformerPickup::BeginPlay()
{
	Super::BeginPlay();
	ApplyPickupSettings();
	MeshBaseZ = Mesh->GetRelativeLocation().Z;
	Clock = FMath::FRand() * 10.f;   // monete vicine non ondeggiano tutte insieme
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &APlatformerPickup::HandleOverlap);
}

void APlatformerPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Delta Seconds: stessa velocità a qualunque frame rate
	Clock += DeltaSeconds;
	Mesh->AddLocalRotation(FRotator(0.f, SpinSpeed * DeltaSeconds, 0.f));
	FVector Loc = Mesh->GetRelativeLocation();
	Loc.Z = MeshBaseZ + FMath::Sin(Clock * BobSpeed * 2.f * PI) * BobHeight;
	Mesh->SetRelativeLocation(Loc);
}

void APlatformerPickup::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// il cast: entriamo nella "scocca" del personaggio
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (bCollected || !Character || !Character->IsPlayerControlled())
	{
		return;
	}
	if (!Collect(Character))
	{
		return;
	}
	bCollected = true;
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}
	OnPickedUp(Character);
	Destroy();
}

// ---------------------------------------------------------------------
// ACoinPickup
// ---------------------------------------------------------------------

ACoinPickup::ACoinPickup()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Mesh->SetStaticMesh(Cylinder.Object);
	}
	// una moneta "in piedi": cilindro schiacciato e ruotato
	Mesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.08f));
	Mesh->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
	GlowIntensity = 4.f;
}

bool ACoinPickup::Collect(ACharacter* Character)
{
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->AddPendingCoins(Value);
	}
	return true;
}

// ---------------------------------------------------------------------
// AAbilityPickup
// ---------------------------------------------------------------------

AAbilityPickup::AAbilityPickup()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (Cone.Succeeded())
	{
		Mesh->SetStaticMesh(Cone.Object);
	}
	Mesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.9f));
	PickupRadius = 80.f;
	GlowIntensity = 30.f;
	GlowColor = FLinearColor(0.3f, 0.7f, 1.f);
	SpinSpeed = 90.f;
	BobHeight = 20.f;
	Label = FText::FromString(TEXT("DOPPIO SALTO"));
}

void AAbilityPickup::BeginPlay()
{
	Super::BeginPlay();
	const UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	if (bHideIfAlreadyUnlocked && Run && Run->HasAbility(Ability))
	{
		Destroy();
	}
}

bool AAbilityPickup::Collect(ACharacter* Character)
{
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->UnlockAbility(Ability);
		Run->PushMessage(Message, FLinearColor(0.4f, 0.8f, 1.f));
	}
	return true;
}

// ---------------------------------------------------------------------
// AHealthPickup
// ---------------------------------------------------------------------

AHealthPickup::AHealthPickup()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
	Mesh->SetRelativeScale3D(FVector(0.4f));
	GlowColor = FLinearColor(0.3f, 1.f, 0.4f);
	GlowIntensity = 10.f;
}

bool AHealthPickup::Collect(ACharacter* Character)
{
	UPlatformerHealthComponent* Health = UPlatformerHealthComponent::FindOrAdd(Character);
	if (Health && Health->GetHealth() >= Health->MaxHealth)
	{
		return false;   // vita piena: la lasciamo lì per dopo
	}
	if (Health)
	{
		Health->Heal(HealAmount);
	}
	// Un "danno negativo" cura anche la barra della vita di BP_Bot2 (Event AnyDamage).
	UGameplayStatics::ApplyDamage(Character, -HealAmount, nullptr, this, UDamageType::StaticClass());
	return true;
}
