#include "PlatformerZones.h"

#include "PlatformerSafeSpotComponent.h"

#include "LevelTimer.h"
#include "EngineUtils.h"
#include "PlatformerHealthComponent.h"
#include "PlatformerRunSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Activable.h"

// ---------------------------------------------------------------------
// AKillZone
// ---------------------------------------------------------------------

void AKillZone::OnCharacterEntered(ACharacter* Character)
{
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->KillPlayerAndRestart(RestartDelay);
	}
}

// ---------------------------------------------------------------------
// AFallZone
// ---------------------------------------------------------------------

void AFallZone::OnCharacterEntered(ACharacter* Character)
{
	if (UPlatformerSafeSpotComponent* Safe = Character ? Character->FindComponentByClass<UPlatformerSafeSpotComponent>() : nullptr)
	{
		Safe->HandleFall();
	}
	else if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->KillPlayerAndRestart(0.2f);   // senza il componente: come una trappola
	}
}

// ---------------------------------------------------------------------
// ADamageZone
// ---------------------------------------------------------------------

void ADamageZone::OnCharacterEntered(ACharacter* Character)
{
	Victim = Character;
	DealDamage();   // il primo colpo arriva subito
	GetWorldTimerManager().SetTimer(DamageHandle, this, &ADamageZone::DealDamage, HitInterval, true);
}

void ADamageZone::OnCharacterExited(ACharacter* Character)
{
	if (Victim.Get() == Character)
	{
		Victim.Reset();
		GetWorldTimerManager().ClearTimer(DamageHandle);
	}
}

void ADamageZone::DealDamage()
{
	ACharacter* Target = Victim.Get();
	if (!Target)
	{
		GetWorldTimerManager().ClearTimer(DamageHandle);
		return;
	}
	UPlatformerHealthComponent* Health = UPlatformerHealthComponent::FindOrAdd(Target);
	// UE11: Apply Damage. Lo ricevono sia il nostro componente vita sia l'Event
	// AnyDamage di BP_Bot2 (che aggiorna la sua barra della vita).
	UGameplayStatics::ApplyDamage(Target, DamagePerHit, nullptr, this, UDamageType::StaticClass());
	if (Health && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1401, 1.f, FColor::Orange,
			FString::Printf(TEXT("Vita: %.0f / %.0f"), Health->GetHealth(), Health->MaxHealth));
	}
}

void ADamageZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DamageHandle);
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------
// AModifierZone
// ---------------------------------------------------------------------

void AModifierZone::OnCharacterEntered(ACharacter* Character)
{
	UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Move)
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(RestoreHandle);

	// Salviamo i valori originali solo la prima volta: se si rientra mentre
	// l'effetto è ancora attivo, non si salvano i valori già modificati.
	if (!bApplied)
	{
		SavedWalkSpeed = Move->MaxWalkSpeed;
		SavedJumpZ = Move->JumpZVelocity;
		SavedGravity = Move->GravityScale;
		SavedFriction = Move->GroundFriction;
		SavedBraking = Move->BrakingDecelerationWalking;
		Move->GroundFriction = SavedFriction * GroundFrictionMultiplier;
		Move->BrakingDecelerationWalking = SavedBraking * BrakingMultiplier;
		Move->MaxWalkSpeed = SavedWalkSpeed * WalkSpeedMultiplier;
		Move->JumpZVelocity = SavedJumpZ * JumpMultiplier;
		Move->GravityScale = SavedGravity * GravityMultiplier;
		bApplied = true;
		Affected = Character;
	}
}

void AModifierZone::OnCharacterExited(ACharacter* Character)
{
	if (!bApplied || Affected.Get() != Character)
	{
		return;
	}
	if (DurationAfterExit < 0.f)
	{
		return;   // effetto permanente
	}
	if (DurationAfterExit == 0.f)
	{
		Restore();
	}
	else
	{
		GetWorldTimerManager().SetTimer(RestoreHandle, this, &AModifierZone::Restore, DurationAfterExit, false);
	}
}

void AModifierZone::Restore()
{
	ACharacter* Character = Affected.Get();
	if (bApplied && Character)
	{
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->MaxWalkSpeed = SavedWalkSpeed;
			Move->JumpZVelocity = SavedJumpZ;
			Move->GravityScale = SavedGravity;
			Move->GroundFriction = SavedFriction;
			Move->BrakingDecelerationWalking = SavedBraking;
		}
	}
	bApplied = false;
	Affected.Reset();
}

void AModifierZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RestoreHandle);
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------
// ALevelGoal
// ---------------------------------------------------------------------

void ALevelGoal::OnCharacterEntered(ACharacter* Character)
{
	if (bReached)
	{
		return;
	}
	bReached = true;
	// arrivati: si "incassano" le monete, si registra il tempo e si salva (GD16)
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->CompleteLevel();
	}
	// e si ferma l'eventuale timer del livello
	for (TActorIterator<ALevelTimer> It(GetWorld()); It; ++It)
	{
		It->StopTimer();
	}
	if (GEngine && !Message.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(-1, Delay + 1.f, FColor::Green, Message);
	}
	if (Delay > 0.f)
	{
		GetWorldTimerManager().SetTimer(GoalHandle, this, &ALevelGoal::OpenNext, Delay, false);
	}
	else
	{
		OpenNext();
	}
}

void ALevelGoal::OpenNext()
{
	UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	if (!Run)
	{
		return;
	}
	if (bResetDeathCount)
	{
		Run->ResetDeathCount();
	}
	if (NextLevel.IsNone())
	{
		Run->RestartCurrentLevel();
	}
	else
	{
		Run->GoToLevel(NextLevel);
	}
}

// ---------------------------------------------------------------------
// ATeleportZone
// ---------------------------------------------------------------------

void ATeleportZone::OnCharacterEntered(ACharacter* Character)
{
	const FVector Target = GetActorTransform().TransformPosition(Destination);
	Character->TeleportTo(Target, Character->GetActorRotation());
	if (bStopVelocity)
	{
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->StopMovementImmediately();
		}
	}
}

// ---------------------------------------------------------------------
// AShopZone
// ---------------------------------------------------------------------

void AShopZone::BeginPlay()
{
	Super::BeginPlay();
	if (Label.IsEmpty())
	{
		Label = FText::FromString(FString::Printf(TEXT("Servono %d monete"), Cost));
		ApplyZoneSettings();
	}
}

void AShopZone::OnCharacterEntered(ACharacter* Character)
{
	if (bPaid && bOnlyOnce)
	{
		return;
	}
	UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	if (!Run)
	{
		return;
	}
	if (!Run->SpendCoins(Cost))
	{
		Run->PushMessage(FString::Printf(TEXT("Ti servono %d monete (ne hai %d)"), Cost, Run->GetVisibleCoins()),
			FLinearColor(1.f, 0.6f, 0.2f));
		return;
	}
	bPaid = true;
	Run->PushMessage(FString::Printf(TEXT("Pagato: %d monete"), Cost), FLinearColor(1.f, 0.85f, 0.2f));
	for (AActivable* Target : Targets)
	{
		if (Target)
		{
			Target->SetActive(true);
		}
	}
	LabelText->SetText(FText::FromString(TEXT("Aperto!")));
}
