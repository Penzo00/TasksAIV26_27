#include "PlatformerRunSubsystem.h"

#include "PlatformerSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

const FString UPlatformerRunSubsystem::SaveSlotName = TEXT("AIV_Platformer");

void UPlatformerRunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreateSave();
}

UPlatformerRunSubsystem* UPlatformerRunSubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext || !GEngine)
	{
		return nullptr;
	}
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UPlatformerRunSubsystem>() : nullptr;
}

FName UPlatformerRunSubsystem::AbilityToName(EPlatformerAbility Ability)
{
	switch (Ability)
	{
	case EPlatformerAbility::DoubleJump: return FName(TEXT("DoubleJump"));
	case EPlatformerAbility::WallRun:    return FName(TEXT("WallRun"));
	case EPlatformerAbility::Jetpack:    return FName(TEXT("Jetpack"));
	case EPlatformerAbility::CrateSpawner: return FName(TEXT("CrateSpawner"));
	default:                             return NAME_None;
	}
}

// ---------------------------------------------------------------------
// Salvataggio (GD16): all'avvio controlla, carica o crea
// ---------------------------------------------------------------------

void UPlatformerRunSubsystem::LoadOrCreateSave()
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		Save = Cast<UPlatformerSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
	}
	if (!Save)
	{
		Save = Cast<UPlatformerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlatformerSaveGame::StaticClass()));
	}
	BankedCoins = Save ? Save->Coins : 0;
}

void UPlatformerRunSubsystem::SaveProgress()
{
	if (!Save)
	{
		return;
	}
	Save->Coins = BankedCoins;
	UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
}

void UPlatformerRunSubsystem::ResetProgress()
{
	UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0);
	Save = Cast<UPlatformerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlatformerSaveGame::StaticClass()));
	BankedCoins = 0;
	PendingCoins = 0;
	DeathCount = 0;
	OnCoinsChanged.Broadcast(GetVisibleCoins());
	PushMessage(TEXT("Salvataggio azzerato"), FLinearColor::Yellow);
}

// ---------------------------------------------------------------------
// Livelli
// ---------------------------------------------------------------------

FName UPlatformerRunSubsystem::CurrentLevelKey() const
{
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	return World ? FName(*UGameplayStatics::GetCurrentLevelName(World, true)) : NAME_None;
}

void UPlatformerRunSubsystem::NotifyLevelStarted(UWorld* World)
{
	// Le monete raccolte e non "incassate" si perdono: il livello è ripartito
	// (o è nuovo) e le monete sono di nuovo al loro posto.
	PendingCoins = 0;
	bRestartPending = false;
	LevelStartTime = World ? World->GetTimeSeconds() : 0.0;
	LevelEndTime = -1.0;
	OnCoinsChanged.Broadcast(GetVisibleCoins());
}

float UPlatformerRunSubsystem::GetLevelElapsedTime() const
{
	if (LevelEndTime >= 0.0)
	{
		return static_cast<float>(LevelEndTime - LevelStartTime);
	}
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	return World ? static_cast<float>(World->GetTimeSeconds() - LevelStartTime) : 0.f;
}

float UPlatformerRunSubsystem::GetBestTimeForCurrentLevel() const
{
	if (!Save)
	{
		return 0.f;
	}
	const float* Best = Save->BestTimes.Find(CurrentLevelKey());
	return Best ? *Best : 0.f;
}

void UPlatformerRunSubsystem::KillPlayerAndRestart(float Delay)
{
	if (bRestartPending)
	{
		return;   // una sola morte per volta, anche se si toccano due trappole insieme
	}
	bRestartPending = true;
	++DeathCount;
	if (Save)
	{
		++Save->TotalDeaths;
	}
	PushMessage(FString::Printf(TEXT("Morto! Morti: %d"), DeathCount), FLinearColor(1.f, 0.25f, 0.2f));

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World && Delay > 0.f)
	{
		World->GetTimerManager().SetTimer(RestartHandle, this,
			&UPlatformerRunSubsystem::RestartCurrentLevel, Delay, false);
	}
	else
	{
		RestartCurrentLevel();
	}
}

void UPlatformerRunSubsystem::RestartCurrentLevel()
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	bRestartPending = false;
	const FString Current = UGameplayStatics::GetCurrentLevelName(World, true);
	UGameplayStatics::OpenLevel(World, FName(*Current));
}

void UPlatformerRunSubsystem::GoToLevel(FName LevelName)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || LevelName.IsNone())
	{
		return;
	}
	bRestartPending = false;
	Lives = MaxLives;   // livello nuovo: vite piene (riaprire lo STESSO livello invece le lascia come sono)
	UGameplayStatics::OpenLevel(World, LevelName);
}

// ---------------------------------------------------------------------
// Vite
// ---------------------------------------------------------------------

bool UPlatformerRunSubsystem::LoseLife()
{
	if (bRestartPending)
	{
		return false;
	}
	Lives = FMath::Max(0, Lives - 1);
	++DeathCount;
	if (Save)
	{
		++Save->TotalDeaths;
	}
	if (Lives > 0)
	{
		PushMessage(FString::Printf(TEXT("Caduto! Vite rimaste: %d"), Lives), FLinearColor(1.f, 0.6f, 0.2f));
		return true;
	}
	GameOver();
	return false;
}

void UPlatformerRunSubsystem::GameOver()
{
	if (bRestartPending)
	{
		return;
	}
	bRestartPending = true;
	PushMessage(TEXT("GAME OVER - si ricomincia dal primo livello"), FLinearColor(1.f, 0.2f, 0.15f));
	PendingCoins = 0;
	if (bGameOverResetsAbilities && Save)
	{
		Save->UnlockedAbilities.Empty();
		SaveProgress();
	}
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World)
	{
		World->GetTimerManager().SetTimer(RestartHandle, this, &UPlatformerRunSubsystem::OpenFirstLevel, 2.f, false);
	}
	else
	{
		OpenFirstLevel();
	}
}

void UPlatformerRunSubsystem::OpenFirstLevel()
{
	GoToLevel(FirstLevel);
}

void UPlatformerRunSubsystem::StartMode(bool bBlueprints, FName FirstLevelOfMode)
{
	bBlueprintMode = bBlueprints;
	if (!FirstLevelOfMode.IsNone())
	{
		FirstLevel = FirstLevelOfMode;   // il game over riporta al primo livello DELLA MODALITÀ scelta
	}
	PendingCoins = 0;
	DeathCount = 0;
	GoToLevel(FirstLevel);
}

void UPlatformerRunSubsystem::GoToMenu()
{
	PendingCoins = 0;   // le monete del livello lasciato a metà non si incassano
	GoToLevel(MenuLevel);
}

void UPlatformerRunSubsystem::CompleteLevel()
{
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	LevelEndTime = World ? World->GetTimeSeconds() : LevelStartTime;
	const float Elapsed = GetLevelElapsedTime();

	BankedCoins += PendingCoins;
	PendingCoins = 0;

	if (Save)
	{
		const FName Key = CurrentLevelKey();
		float& Best = Save->BestTimes.FindOrAdd(Key);
		if (Best <= 0.f || Elapsed < Best)
		{
			Best = Elapsed;
			PushMessage(FString::Printf(TEXT("Nuovo record: %.2f s"), Elapsed), FLinearColor(1.f, 0.85f, 0.2f));
		}
	}
	SaveProgress();
	OnCoinsChanged.Broadcast(GetVisibleCoins());
}

// ---------------------------------------------------------------------
// Monete e abilità
// ---------------------------------------------------------------------

void UPlatformerRunSubsystem::AddPendingCoins(int32 Amount)
{
	PendingCoins = FMath::Max(0, PendingCoins + Amount);
	OnCoinsChanged.Broadcast(GetVisibleCoins());
}

bool UPlatformerRunSubsystem::SpendCoins(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}
	if (GetVisibleCoins() < Amount)
	{
		return false;
	}
	const int32 FromPending = FMath::Min(PendingCoins, Amount);
	PendingCoins -= FromPending;
	BankedCoins -= (Amount - FromPending);
	SaveProgress();
	OnCoinsChanged.Broadcast(GetVisibleCoins());
	return true;
}

bool UPlatformerRunSubsystem::HasAbility(EPlatformerAbility Ability) const
{
	return Save && Save->UnlockedAbilities.Contains(AbilityToName(Ability));
}

void UPlatformerRunSubsystem::UnlockAbility(EPlatformerAbility Ability)
{
	if (!Save || HasAbility(Ability))
	{
		return;
	}
	Save->UnlockedAbilities.Add(AbilityToName(Ability));
	SaveProgress();
	OnAbilityUnlocked.Broadcast(Ability);
}

void UPlatformerRunSubsystem::PushMessage(const FString& Message, FLinearColor Color)
{
	OnMessage.Broadcast(Message, Color);
}
