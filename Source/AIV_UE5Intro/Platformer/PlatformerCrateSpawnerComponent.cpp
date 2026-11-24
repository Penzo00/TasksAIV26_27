#include "PlatformerCrateSpawnerComponent.h"

#include "PlatformerGym.h"
#include "PlatformerRuntimeInput.h"
#include "PlatformerRunSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "TimerManager.h"

UPlatformerCrateSpawnerComponent::UPlatformerCrateSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;   // serve solo a legare l'input appena il pawn è posseduto
	CrateClass = ASpawnedCrate::StaticClass();
	SpawnKeys = { EKeys::F, EKeys::Gamepad_FaceButton_Left };
}

void UPlatformerCrateSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	Charges = MaxCharges;
	SpawnAction = PlatformerRuntimeInput::MakeAction(this, TEXT("IA_Platformer_SpawnCrate"), EInputActionValueType::Boolean);
	SpawnContext = PlatformerRuntimeInput::MakeContext(this, SpawnAction, SpawnKeys);
	TryBindInput();
}

void UPlatformerCrateSpawnerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TryBindInput();
}

void UPlatformerCrateSpawnerComponent::TryBindInput()
{
	if (!Character)
	{
		return;
	}
	if (!bContextAdded)
	{
		bContextAdded = PlatformerRuntimeInput::AddContext(Character, SpawnContext, 5);
	}
	if (BoundInput.IsValid())
	{
		return;
	}
	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Character->InputComponent);
	if (!Input)
	{
		return;
	}
	Input->BindAction(SpawnAction, ETriggerEvent::Started, this, &UPlatformerCrateSpawnerComponent::HandleSpawnPressed);
	BoundInput = Input;
}

bool UPlatformerCrateSpawnerComponent::IsUnlocked() const
{
	if (bAlwaysUnlocked)
	{
		return true;
	}
	const UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	return Run && Run->HasAbility(EPlatformerAbility::CrateSpawner);
}

float UPlatformerCrateSpawnerComponent::GetRechargeTimeLeft() const
{
	const UWorld* World = GetWorld();
	if (!World || !World->GetTimerManager().IsTimerActive(RechargeHandle))
	{
		return 0.f;
	}
	return World->GetTimerManager().GetTimerRemaining(RechargeHandle);
}

void UPlatformerCrateSpawnerComponent::HandleSpawnPressed()
{
	SpawnCrate();
}

bool UPlatformerCrateSpawnerComponent::SpawnCrate()
{
	UWorld* World = GetWorld();
	if (!World || !Character || !IsUnlocked() || !CrateClass)
	{
		return false;
	}
	UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	if (Charges <= 0)
	{
		if (Run)
		{
			Run->PushMessage(FString::Printf(TEXT("Casse in ricarica: %.1f s"), GetRechargeTimeLeft()), FLinearColor(1.f, 0.7f, 0.3f));
		}
		return false;
	}

	// il punto davanti al personaggio; un trace dall'alto trova il pavimento o la cassa già lì
	const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
	const FVector Base = Character->GetActorLocation() + Forward * SpawnDistance;
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Start = Base + FVector(0.f, 0.f, CrateSize * 3.f);
	const FVector End = Base - FVector(0.f, 0.f, HalfHeight + 600.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlatformerCrateSpawn), false, Character);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) || Hit.ImpactNormal.Z < 0.6f)
	{
		if (Run)
		{
			Run->PushMessage(TEXT("Qui non c'e' un pavimento per la cassa"), FLinearColor(1.f, 0.7f, 0.3f));
		}
		return false;
	}

	const FVector Location = Hit.ImpactPoint + FVector(0.f, 0.f, CrateSize * 0.5f + 0.5f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASpawnedCrate* Crate = World->SpawnActor<ASpawnedCrate>(CrateClass, Location, FRotator(0.f, Character->GetActorRotation().Yaw, 0.f), SpawnParams);
	if (!Crate)
	{
		return false;
	}
	Crate->SetCrate(CrateSize, CrateMaterial);
	if (CrateLifetime > 0.f)
	{
		Crate->SetLifeSpan(CrateLifetime);
	}

	// troppe casse: via la più vecchia
	Alive.RemoveAll([](const TWeakObjectPtr<ASpawnedCrate>& C) { return !C.IsValid(); });
	Alive.Add(Crate);
	while (Alive.Num() > MaxAliveCrates)
	{
		if (ASpawnedCrate* Old = Alive[0].Get())
		{
			Old->Destroy();
		}
		Alive.RemoveAt(0);
	}

	--Charges;
	if (Charges <= 0)
	{
		World->GetTimerManager().SetTimer(RechargeHandle, this, &UPlatformerCrateSpawnerComponent::Recharge, RechargeTime, false);
	}
	return true;
}

void UPlatformerCrateSpawnerComponent::Recharge()
{
	Charges = MaxCharges;
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->PushMessage(FString::Printf(TEXT("Casse pronte: %d"), Charges), FLinearColor(0.6f, 1.f, 0.6f));
	}
}
