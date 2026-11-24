#include "PlatformerTorchComponent.h"

#include "PlatformerRuntimeInput.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

UPlatformerTorchComponent::UPlatformerTorchComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlatformerTorchComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	Arm = Character ? Character->FindComponentByClass<USpringArmComponent>() : nullptr;
	Camera = Character ? Character->FindComponentByClass<UCameraComponent>() : nullptr;
	if (!Character || !Arm || !Camera)
	{
		SetComponentTickEnabled(false);
		return;
	}
	StartArmLength = Arm->TargetArmLength;
	StartTargetOffset = Arm->TargetOffset;
	DesiredArmLength = StartArmLength;
	if (MaxArmLength <= 0.f)
	{
		MaxArmLength = StartArmLength;
	}
	Battery = MaxBattery;

	// l'azione della rotellina, creata da codice se non è stata assegnata
	if (!ZoomAction)
	{
		ZoomAction = PlatformerRuntimeInput::MakeAction(this, TEXT("IA_Platformer_Zoom"), EInputActionValueType::Axis1D);
	}
	ZoomContext = PlatformerRuntimeInput::MakeContext(this, ZoomAction, { EKeys::MouseWheelAxis });

	CreateTorch();
	TryBindInput();
}

void UPlatformerTorchComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bFirstPerson)
	{
		SetFirstPerson(false);
	}
	Super::EndPlay(EndPlayReason);
}

void UPlatformerTorchComponent::CreateTorch()
{
	// la torcia è attaccata alla TELECAMERA: in prima persona si vede in basso a destra
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	TorchMesh = NewObject<UStaticMeshComponent>(Character, TEXT("TorchMesh"));
	TorchMesh->SetStaticMesh(Cylinder);
	TorchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TorchMesh->SetCastShadow(false);
	TorchMesh->SetRelativeLocation(FVector(45.f, 20.f, -18.f));
	TorchMesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));   // l'asse del cilindro guarda in avanti
	TorchMesh->SetRelativeScale3D(FVector(0.06f, 0.06f, 0.26f));
	if (TorchMaterial)
	{
		TorchMesh->SetMaterial(0, TorchMaterial);
	}
	TorchMesh->SetVisibility(false);
	TorchMesh->RegisterComponent();
	TorchMesh->AttachToComponent(Camera, FAttachmentTransformRules::KeepRelativeTransform);

	TorchLight = NewObject<USpotLightComponent>(Character, TEXT("TorchLight"));
	TorchLight->SetRelativeLocation(FVector(60.f, 20.f, -18.f));
	TorchLight->SetIntensityUnits(ELightUnits::Candelas);
	TorchLight->SetIntensity(LightIntensity);
	TorchLight->SetAttenuationRadius(LightRange);
	TorchLight->SetOuterConeAngle(ConeAngle);
	TorchLight->SetInnerConeAngle(ConeAngle * 0.55f);
	TorchLight->SetLightColor(LightColor);
	TorchLight->SetSourceRadius(2.f);
	TorchLight->SetCastShadows(true);
	TorchLight->SetVisibility(false);
	TorchLight->RegisterComponent();
	TorchLight->AttachToComponent(Camera, FAttachmentTransformRules::KeepRelativeTransform);
}

void UPlatformerTorchComponent::TryBindInput()
{
	if (!Character)
	{
		return;
	}
	if (!bContextAdded)
	{
		bContextAdded = PlatformerRuntimeInput::AddContext(Character, ZoomContext, 5);
	}
	if (BoundInput.IsValid())
	{
		return;
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Character->InputComponent);
	if (!Input || !ZoomAction)
	{
		return;
	}
	Input->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &UPlatformerTorchComponent::HandleZoom);
	BoundInput = Input;
}

void UPlatformerTorchComponent::HandleZoom(const FInputActionValue& Value)
{
	Zoom(Value.Get<float>());
}

void UPlatformerTorchComponent::Zoom(float Steps)
{
	// rotellina in avanti (valore positivo) = telecamera più vicina
	DesiredArmLength = FMath::Clamp(DesiredArmLength - Steps * ZoomStep, 0.f, MaxArmLength);
}

void UPlatformerTorchComponent::AddBattery(float Seconds)
{
	Battery = FMath::Clamp(Battery + Seconds, 0.f, MaxBattery);
}

void UPlatformerTorchComponent::SetFirstPerson(bool bEnable)
{
	if (bFirstPerson == bEnable || !Character)
	{
		return;
	}
	bFirstPerson = bEnable;
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (bEnable)
	{
		// il corpo (e quello che ha attaccato) non si vede da dentro;
		// il personaggio gira con la telecamera
		HiddenComponents.Reset();
		TArray<UPrimitiveComponent*> Primitives;
		Character->GetComponents<UPrimitiveComponent>(Primitives);
		for (UPrimitiveComponent* Prim : Primitives)
		{
			if (Prim && Prim != TorchMesh && Prim != Character->GetRootComponent() && !Prim->bOwnerNoSee)
			{
				Prim->SetOwnerNoSee(true);
				HiddenComponents.Add(Prim);
			}
		}
		bSavedUseControllerYaw = Character->bUseControllerRotationYaw;
		Character->bUseControllerRotationYaw = true;
		if (Move)
		{
			bSavedOrientToMovement = Move->bOrientRotationToMovement;
			Move->bOrientRotationToMovement = false;
		}
		Arm->TargetOffset = StartTargetOffset + FVector(0.f, 0.f, EyeHeight);
	}
	else
	{
		for (const TWeakObjectPtr<UPrimitiveComponent>& Prim : HiddenComponents)
		{
			if (Prim.IsValid())
			{
				Prim->SetOwnerNoSee(false);
			}
		}
		HiddenComponents.Reset();
		Character->bUseControllerRotationYaw = bSavedUseControllerYaw;
		if (Move)
		{
			Move->bOrientRotationToMovement = bSavedOrientToMovement;
		}
		Arm->TargetOffset = StartTargetOffset;
	}
	TorchMesh->SetVisibility(bEnable);
}

void UPlatformerTorchComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Character || !Arm)
	{
		return;
	}
	TryBindInput();

	// il braccio della telecamera raggiunge piano la lunghezza voluta
	Arm->TargetArmLength = FMath::FInterpTo(Arm->TargetArmLength, DesiredArmLength, DeltaTime, ZoomInterpSpeed);
	SetFirstPerson(Arm->TargetArmLength <= FirstPersonThreshold);

	if (bFirstPerson)
	{
		Battery = FMath::Max(0.f, Battery - DeltaTime);
	}
	else
	{
		Battery = FMath::Min(MaxBattery, Battery + RechargePerSecond * DeltaTime);
	}

	const bool bOn = IsLightOn();
	TorchLight->SetVisibility(bOn);
	if (bOn)
	{
		float Intensity = LightIntensity;
		if (GetBatteryPercent() < LowBatteryThreshold)
		{
			// batteria quasi finita: la luce sfarfalla, sempre di più
			const float Noise = FMath::PerlinNoise1D(GetWorld()->GetTimeSeconds() * 9.f);
			const float Weak = GetBatteryPercent() / FMath::Max(LowBatteryThreshold, 0.01f);
			Intensity *= FMath::Clamp(0.35f + 0.65f * Weak + 0.4f * Noise, 0.05f, 1.f);
		}
		TorchLight->SetIntensity(Intensity);
	}
}
