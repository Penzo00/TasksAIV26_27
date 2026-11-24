#include "Turret.h"

#include "PlatformerRunSubsystem.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------------
// APlatformerProjectile
// ---------------------------------------------------------------------

APlatformerProjectile::APlatformerProjectile()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->SetSphereRadius(14.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetNotifyRigidBodyCollision(true);   // "Simulation Generates Hit Events"
	Collision->CanCharacterStepUpOn = ECB_No;
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.28f));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 1500.f;
	Movement->MaxSpeed = 1500.f;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 5.f;   // il proiettile che si distrugge da solo
}

void APlatformerProjectile::BeginPlay()
{
	Super::BeginPlay();
	Collision->OnComponentHit.AddDynamic(this, &APlatformerProjectile::HandleHit);
	if (AActor* Shooter = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
}

void APlatformerProjectile::SetProjectileMaterial(UMaterialInterface* Material)
{
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}
}

void APlatformerProjectile::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (ACharacter* Character = Cast<ACharacter>(OtherActor); Character && bRestartLevelOnHit)
	{
		// la torretta "mortale": colpito = il livello riparte
		if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
		{
			Run->KillPlayerAndRestart(0.4f);
		}
	}
	else if (Character)
	{
		UGameplayStatics::ApplyDamage(Character, Damage, nullptr, this, UDamageType::StaticClass());
		const FVector Push = GetVelocity().GetSafeNormal2D() * Knockback + FVector(0.f, 0.f, Knockback * 0.4f);
		Character->LaunchCharacter(Push, false, false);
	}
	Destroy();
}

// ---------------------------------------------------------------------
// ATurret
// ---------------------------------------------------------------------

ATurret::ATurret()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// La torretta in tre pezzi (UE4): base, testa che gira in yaw, canna che gira in pitch.
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	Base->SetupAttachment(Root);
	Base->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.0f));
	Base->SetRelativeLocation(FVector(0.f, 0.f, 50.f));

	Head = CreateDefaultSubobject<USceneComponent>(TEXT("Head"));
	Head->SetupAttachment(Root);
	Head->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	Head->SetMobility(EComponentMobility::Movable);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(Head);
	HeadMesh->SetRelativeScale3D(FVector(0.75f));
	HeadMesh->SetMobility(EComponentMobility::Movable);

	Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Barrel"));
	Barrel->SetupAttachment(Head);
	Barrel->SetMobility(EComponentMobility::Movable);
	Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Head);
	Muzzle->SetRelativeLocation(FVector(110.f, 0.f, 0.f));
	Muzzle->SetMobility(EComponentMobility::Movable);

	if (Cylinder.Succeeded())
	{
		Base->SetStaticMesh(Cylinder.Object);
		Barrel->SetStaticMesh(Cylinder.Object);
	}
	if (SphereMesh.Succeeded())
	{
		HeadMesh->SetStaticMesh(SphereMesh.Object);
	}

	Detection = CreateDefaultSubobject<USphereComponent>(TEXT("Detection"));
	Detection->SetupAttachment(Root);
	Detection->SetCollisionProfileName(TEXT("Trigger"));

	ProjectileClass = APlatformerProjectile::StaticClass();
}

void ATurret::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyTurretSettings();
}

void ATurret::ApplyTurretSettings()
{
	Detection->SetSphereRadius(Range);
	// la canna è un cilindro coricato lungo X, che parte dal centro della testa
	Barrel->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f) + FRotator(CurrentPitch, 0.f, 0.f));
	Barrel->SetRelativeScale3D(FVector(0.22f, 0.22f, 1.f));
	Barrel->SetRelativeLocation(FRotator(CurrentPitch, 0.f, 0.f).RotateVector(FVector(55.f, 0.f, 0.f)));
	if (BodyMaterial)
	{
		Base->SetMaterial(0, BodyMaterial);
		HeadMesh->SetMaterial(0, BodyMaterial);
		Barrel->SetMaterial(0, BodyMaterial);
	}
}

void ATurret::BeginPlay()
{
	Super::BeginPlay();
	ApplyTurretSettings();
	Detection->OnComponentBeginOverlap.AddDynamic(this, &ATurret::HandleBeginOverlap);
	Detection->OnComponentEndOverlap.AddDynamic(this, &ATurret::HandleEndOverlap);

	// il player potrebbe essere già dentro il raggio all'avvio
	TArray<AActor*> Inside;
	Detection->GetOverlappingActors(Inside, ACharacter::StaticClass());
	for (AActor* Actor : Inside)
	{
		ACharacter* Character = Cast<ACharacter>(Actor);
		if (Character && Character->IsPlayerControlled())
		{
			Target = Character;
			GetWorldTimerManager().SetTimer(FireHandle, this, &ATurret::Shoot, FireInterval, true, FireInterval);
		}
	}
}

void ATurret::HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character || !Character->IsPlayerControlled())
	{
		return;
	}
	Target = Character;
	// Set Timer by Event: spara ogni FireInterval secondi, finché il bersaglio è nel raggio
	GetWorldTimerManager().SetTimer(FireHandle, this, &ATurret::Shoot, FireInterval, true, FireInterval * 0.5f);
}

void ATurret::HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == Target.Get())
	{
		Target.Reset();
		GetWorldTimerManager().ClearTimer(FireHandle);   // la torretta si ferma
	}
}

FVector ATurret::GetAimPoint() const
{
	const ACharacter* Character = Target.Get();
	if (!Character)
	{
		return Muzzle->GetComponentLocation() + Head->GetForwardVector() * 100.f;
	}
	FVector Point = Character->GetActorLocation() + FVector(0.f, 0.f, AimHeightOffset);
	if (bLeadTarget)
	{
		const float Time = FVector::Distance(Muzzle->GetComponentLocation(), Point) / ProjectileSpeed;
		Point += Character->GetVelocity() * Time;
	}
	return Point;
}

bool ATurret::HasLineOfSight() const
{
	const ACharacter* Character = Target.Get();
	if (!Character)
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TurretSight), false, this);
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Muzzle->GetComponentLocation(),
		Character->GetActorLocation(), ECC_Visibility, Params);
	return !bBlocked || Hit.GetActor() == Character;
}

void ATurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Target.IsValid())
	{
		return;
	}
	// Find Look at Rotation dalla testa al bersaglio, poi RInterp To per girare piano
	const FRotator Look = UKismetMathLibrary::FindLookAtRotation(Head->GetComponentLocation(), GetAimPoint());
	const FRotator Current(0.f, Head->GetComponentRotation().Yaw, 0.f);
	const FRotator NewYaw = FMath::RInterpTo(Current, FRotator(0.f, Look.Yaw, 0.f), DeltaSeconds, TurnSpeed);
	Head->SetWorldRotation(NewYaw);

	// il pitch va sulla canna, con il Clamp
	const float WantedPitch = FMath::Clamp(Look.Pitch, MinPitch, MaxPitch);
	CurrentPitch = FMath::FInterpTo(CurrentPitch, WantedPitch, DeltaSeconds, TurnSpeed);
	Barrel->SetRelativeRotation(FRotator(-90.f + CurrentPitch, 0.f, 0.f));
	Barrel->SetRelativeLocation(FRotator(CurrentPitch, 0.f, 0.f).RotateVector(FVector(55.f, 0.f, 0.f)));
	Muzzle->SetRelativeLocation(FRotator(CurrentPitch, 0.f, 0.f).RotateVector(FVector(110.f, 0.f, 0.f)));
}

void ATurret::Shoot()
{
	if (!Target.IsValid() || !HasLineOfSight() || !ProjectileClass)
	{
		return;
	}
	const FVector From = Muzzle->GetComponentLocation();
	const FVector Dir = (GetAimPoint() - From).GetSafeNormal();
	const FVector Facing = FRotator(CurrentPitch, Head->GetComponentRotation().Yaw, 0.f).Vector();
	if (FVector::DotProduct(Dir, Facing) < FMath::Cos(FMath::DegreesToRadians(FireConeDegrees)))
	{
		return;   // non è ancora girata verso il bersaglio
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlatformerProjectile* Bullet = GetWorld()->SpawnActor<APlatformerProjectile>(ProjectileClass, From, Dir.Rotation(), Params);
	if (!Bullet)
	{
		return;
	}
	Bullet->Damage = ProjectileDamage;
	Bullet->bRestartLevelOnHit = bRestartLevelOnHit;
	Bullet->SetProjectileMaterial(ProjectileMaterial);
	if (UProjectileMovementComponent* Move = Bullet->GetMovement())
	{
		Move->Velocity = Dir * ProjectileSpeed;
		Move->MaxSpeed = ProjectileSpeed;
	}
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, From);
	}
}
