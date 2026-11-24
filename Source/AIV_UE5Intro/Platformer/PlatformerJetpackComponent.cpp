#include "PlatformerJetpackComponent.h"

#include "PlatformerAbilitiesComponent.h"
#include "PlatformerRunSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"

UPlatformerJetpackComponent::UPlatformerJetpackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	JumpAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Input/Actions/IA_Jump.IA_Jump")));
}

void UPlatformerJetpackComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	Move = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !Move)
	{
		SetComponentTickEnabled(false);
		return;
	}
	Fuel = MaxFuel;
	// come per le abilità: il nostro tick prima del Character Movement
	Move->PrimaryComponentTick.AddPrerequisite(this, PrimaryComponentTick);
	CreateFlame();
	TryBindInput();
}

void UPlatformerJetpackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopThrust();
	Super::EndPlay(EndPlayReason);
}

void UPlatformerJetpackComponent::CreateFlame()
{
	// una "fiamma" (cono rovesciato) e una luce dietro la schiena, visibili solo in volo
	USceneComponent* Root = Character->GetRootComponent();
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	Flame = NewObject<UStaticMeshComponent>(Character, TEXT("JetpackFlame"));
	Flame->SetStaticMesh(Cone);
	Flame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Flame->SetCastShadow(false);
	Flame->SetRelativeLocation(FVector(-30.f, 0.f, -10.f));
	Flame->SetRelativeRotation(FRotator(180.f, 0.f, 0.f));   // punta verso il basso
	Flame->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.45f));
	if (FlameMaterial)
	{
		Flame->SetMaterial(0, FlameMaterial);
	}
	Flame->SetVisibility(false);
	Flame->RegisterComponent();
	Flame->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

	FlameLight = NewObject<UPointLightComponent>(Character, TEXT("JetpackLight"));
	FlameLight->SetRelativeLocation(FVector(-35.f, 0.f, -50.f));
	FlameLight->SetLightColor(FLinearColor(1.f, 0.5f, 0.15f));
	FlameLight->SetIntensityUnits(ELightUnits::Candelas);
	FlameLight->SetIntensity(40.f);
	FlameLight->SetAttenuationRadius(450.f);
	FlameLight->SetCastShadows(false);
	FlameLight->SetVisibility(false);
	FlameLight->RegisterComponent();
	FlameLight->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
}

void UPlatformerJetpackComponent::TryBindInput()
{
	if (!Character || BoundInput.IsValid())
	{
		return;
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Character->InputComponent);
	if (!Input)
	{
		return;
	}
	if (UInputAction* Jump = JumpAction.LoadSynchronous())
	{
		Input->BindAction(Jump, ETriggerEvent::Started, this, &UPlatformerJetpackComponent::HandleJumpPressed);
		Input->BindAction(Jump, ETriggerEvent::Completed, this, &UPlatformerJetpackComponent::HandleJumpReleased);
		Input->BindAction(Jump, ETriggerEvent::Canceled, this, &UPlatformerJetpackComponent::HandleJumpReleased);
	}
	BoundInput = Input;
}

bool UPlatformerJetpackComponent::IsUnlocked() const
{
	if (bAlwaysUnlocked)
	{
		return true;
	}
	const UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	return Run && Run->HasAbility(EPlatformerAbility::Jetpack);
}

void UPlatformerJetpackComponent::HandleJumpPressed()
{
	bHeld = true;
	PressedTime = GetWorld()->GetTimeSeconds();
}

void UPlatformerJetpackComponent::HandleJumpReleased()
{
	bHeld = false;
	StopThrust();
}

void UPlatformerJetpackComponent::StartThrust()
{
	if (bThrusting)
	{
		return;
	}
	bThrusting = true;
	SavedAirControl = Move->AirControl;
	Move->AirControl = FMath::Max(Move->AirControl, JetpackAirControl);
	Flame->SetVisibility(true);
	FlameLight->SetVisibility(true);
	OnJetpackStarted.Broadcast();
}

void UPlatformerJetpackComponent::StopThrust()
{
	if (!bThrusting)
	{
		return;
	}
	bThrusting = false;
	if (Move && Move->IsFalling())
	{
		Move->AirControl = SavedAirControl;
	}
	if (Flame)
	{
		Flame->SetVisibility(false);
	}
	if (FlameLight)
	{
		FlameLight->SetVisibility(false);
	}
	OnJetpackStopped.Broadcast();
}

void UPlatformerJetpackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Character || !Move)
	{
		return;
	}
	TryBindInput();

	if (Move->IsMovingOnGround())
	{
		StopThrust();
		Fuel = FMath::Min(MaxFuel, Fuel + RefillPerSecond * DeltaTime);
		return;
	}

	const UPlatformerAbilitiesComponent* Abilities = Character->FindComponentByClass<UPlatformerAbilitiesComponent>();
	const bool bWallRunning = Abilities && Abilities->IsWallRunning();
	const bool bWantsThrust = bHeld && IsUnlocked() && Move->IsFalling() && !bWallRunning
		&& GetWorld()->GetTimeSeconds() - PressedTime >= HoldDelay && Fuel > 0.f;

	if (!bWantsThrust)
	{
		StopThrust();
		return;
	}
	StartThrust();
	Fuel = FMath::Max(0.f, Fuel - DeltaTime);
	Move->Velocity.Z = FMath::FInterpConstantTo(Move->Velocity.Z, MaxUpSpeed, DeltaTime, Acceleration);
	// la fiamma "trema" un po'
	const float Flicker = 0.85f + 0.3f * FMath::PerlinNoise1D(GetWorld()->GetTimeSeconds() * 12.f);
	Flame->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.45f * Flicker));
	FlameLight->SetIntensity(40.f * Flicker);
}
