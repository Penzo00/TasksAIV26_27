#include "PlatformerZones.h"

#include "LevelTimer.h"
#include "EngineUtils.h"
#include "PlatformerHealthComponent.h"
#include "PlatformerRunSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

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
	if (UPlatformerHealthComponent* Health = UPlatformerHealthComponent::FindOrAdd(Target))
	{
		Health->ApplyDamage(DamagePerHit);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1401, 1.f, FColor::Orange,
				FString::Printf(TEXT("Vita: %.0f / %.0f"), Health->GetHealth(), Health->MaxHealth));
		}
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
	// arrivati: si ferma l'eventuale timer del livello
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
