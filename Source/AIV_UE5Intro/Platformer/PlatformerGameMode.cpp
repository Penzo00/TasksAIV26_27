#include "PlatformerGameMode.h"

#include "PlatformerAbilitiesComponent.h"
#include "PlatformerCrateSpawnerComponent.h"
#include "PlatformerJetpackComponent.h"
#include "PlatformerSafeSpotComponent.h"
#include "PlatformerTorchComponent.h"
#include "PlatformerHealthComponent.h"
#include "PlatformerHUD.h"
#include "PlatformerRunSubsystem.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Aggiunge al pawn un componente di quella classe, se non lo ha già (Add Component a runtime). */
	template <typename T>
	T* AddComponentIfMissing(AActor* Actor, const TCHAR* Name)
	{
		if (!Actor)
		{
			return nullptr;
		}
		if (T* Existing = Actor->FindComponentByClass<T>())
		{
			return Existing;
		}
		T* NewComp = NewObject<T>(Actor, Name);
		Actor->AddInstanceComponent(NewComp);
		NewComp->RegisterComponent();
		return NewComp;
	}
}

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
		Abilities->bDoubleJumpAlwaysUnlocked |= bDoubleJumpForTesting || bAllAbilitiesForTesting;
	}
	// le abilità nuove: ognuna è un componente a sé (si possono rifare una per una in Blueprint)
	if (UPlatformerJetpackComponent* Jetpack = AddComponentIfMissing<UPlatformerJetpackComponent>(PlayerPawn, TEXT("PlatformerJetpack")))
	{
		Jetpack->bAlwaysUnlocked |= bAllAbilitiesForTesting;
	}
	if (UPlatformerCrateSpawnerComponent* Crates = AddComponentIfMissing<UPlatformerCrateSpawnerComponent>(PlayerPawn, TEXT("PlatformerCrates")))
	{
		Crates->bAlwaysUnlocked |= bAllAbilitiesForTesting;
	}
	AddComponentIfMissing<UPlatformerTorchComponent>(PlayerPawn, TEXT("PlatformerTorch"));
	AddComponentIfMissing<UPlatformerSafeSpotComponent>(PlayerPawn, TEXT("PlatformerSafeSpot"));
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
		Run->UnlockAbility(EPlatformerAbility::Jetpack);
		Run->UnlockAbility(EPlatformerAbility::CrateSpawner);
		Run->PushMessage(TEXT("Tutte le abilità sbloccate"), FLinearColor(0.4f, 0.8f, 1.f));
	}
}
