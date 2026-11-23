#include "PlatformerGameMode.h"

#include "PlatformerAbilitiesComponent.h"
#include "PlatformerHealthComponent.h"
#include "PlatformerHUD.h"
#include "PlatformerRunSubsystem.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

APlatformerGameMode::APlatformerGameMode()
{
	// Il personaggio controllabile è il Blueprint di Bot2.
	// Se lo spostate o rinominate, cambiate questo percorso
	// (oppure create un Blueprint figlio di questa GameMode e cambiate lì Default Pawn Class).
	static ConstructorHelpers::FClassFinder<APawn> Bot2(TEXT("/Game/Characters/Bot2/BP_Bot2"));
	if (Bot2.Succeeded())
	{
		DefaultPawnClass = Bot2.Class;
	}
	HUDClass = APlatformerHUD::StaticClass();
}

void APlatformerGameMode::StartPlay()
{
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->NotifyLevelStarted(GetWorld());
	}
	Super::StartPlay();
}

void APlatformerGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);
	// I componenti della lezione 13, aggiunti a runtime: vita e abilità.
	UPlatformerHealthComponent::FindOrAdd(PlayerPawn);
	if (UPlatformerAbilitiesComponent* Abilities = UPlatformerAbilitiesComponent::FindOrAdd(PlayerPawn))
	{
		Abilities->bDoubleJumpAlwaysUnlocked |= bDoubleJumpForTesting;
	}
}

void APlatformerGameMode::AIVResetProgress()
{
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->ResetProgress();
	}
}

void APlatformerGameMode::AIVUnlockAll()
{
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->UnlockAbility(EPlatformerAbility::DoubleJump);
		Run->UnlockAbility(EPlatformerAbility::WallRun);
		Run->PushMessage(TEXT("Tutte le abilità sbloccate"), FLinearColor(0.4f, 0.8f, 1.f));
	}
}
