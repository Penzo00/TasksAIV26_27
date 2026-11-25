#include "PlatformerRunSubsystem.h"

#include "PlatformerSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

const FString UPlatformerRunSubsystem::SaveSlotName = TEXT("AIV_Platformer");
const FString UPlatformerRunSubsystem::SaveSlotNameBP = TEXT("AIV_Platformer_BP");
const FName UPlatformerRunSubsystem::DefaultFirstLevelCpp = FName(TEXT("L_GD13_Livello1"));
const FName UPlatformerRunSubsystem::DefaultFirstLevelBP = FName(TEXT("L_GD13_Livello1_BP"));

namespace
{
	/** I livelli Blueprint si riconoscono dal nome: finiscono con _BP. */
	bool IsBlueprintLevelName(const FString& LevelName)
	{
		return LevelName.EndsWith(TEXT("_BP"), ESearchCase::IgnoreCase);
	}

	/** Il livello esiste davvero fra gli asset? LastLevel è un percorso completo, "/Game/Maps/L_Nome". */
	bool LevelExists(FName LevelPath)
	{
		const FString Path = LevelPath.ToString();
		return !LevelPath.IsNone() && Path.StartsWith(TEXT("/")) && FPackageName::DoesPackageExist(Path);
	}
}

void UPlatformerRunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreateSave();
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UPlatformerRunSubsystem::OnPostLoadMap);
}

void UPlatformerRunSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	Super::Deinitialize();
}

FString UPlatformerRunSubsystem::GetSlotNameForMode(bool bBlueprints)
{
	return bBlueprints ? SaveSlotNameBP : SaveSlotName;
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
// Salvataggio (GD16): all'avvio controlla, carica o crea.
// Uno slot per modalità: AIV_Platformer (C++) e AIV_Platformer_BP (Blueprints).
// ---------------------------------------------------------------------

void UPlatformerRunSubsystem::LoadOrCreateSave()
{
	const FString Slot = GetSlotNameForMode(bBlueprintMode);
	Save = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		Save = Cast<UPlatformerSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	}
	if (!Save)
	{
		Save = Cast<UPlatformerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlatformerSaveGame::StaticClass()));
	}
	if (Save)
	{
		Save->bBlueprintMode = bBlueprintMode;
	}
	BankedCoins = Save ? Save->Coins : 0;
	PendingCoins = 0;
	OnCoinsChanged.Broadcast(GetVisibleCoins());
}

void UPlatformerRunSubsystem::SaveProgress()
{
	if (!Save)
	{
		return;
	}
	Save->Coins = BankedCoins;
	Save->bBlueprintMode = bBlueprintMode;
	UGameplayStatics::SaveGameToSlot(Save, GetSlotNameForMode(bBlueprintMode), 0);
}

bool UPlatformerRunSubsystem::HasSaveForMode(bool bBlueprints) const
{
	return UGameplayStatics::DoesSaveGameExist(GetSlotNameForMode(bBlueprints), 0);
}

bool UPlatformerRunSubsystem::GetSaveSummary(bool bBlueprints, FName& OutLastLevel, int32& OutCoins,
	TArray<FName>& OutAbilities) const
{
	OutLastLevel = NAME_None;
	OutCoins = 0;
	OutAbilities.Reset();

	// La modalità in corso è già in memoria (ed è più aggiornata del file).
	const UPlatformerSaveGame* Data = nullptr;
	if (bBlueprints == bBlueprintMode && Save && HasSaveForMode(bBlueprints))
	{
		Data = Save;
	}
	else if (HasSaveForMode(bBlueprints))
	{
		Data = Cast<UPlatformerSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotNameForMode(bBlueprints), 0));
	}
	if (!Data)
	{
		return false;
	}
	OutLastLevel = Data->LastLevel;
	OutCoins = (Data == Save) ? BankedCoins : Data->Coins;
	OutAbilities = Data->UnlockedAbilities;
	return true;
}

void UPlatformerRunSubsystem::ResetProgress()
{
	UGameplayStatics::DeleteGameInSlot(GetSlotNameForMode(bBlueprintMode), 0);
	Save = Cast<UPlatformerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlatformerSaveGame::StaticClass()));
	if (Save)
	{
		Save->bBlueprintMode = bBlueprintMode;
	}
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

// ---------------------------------------------------------------------
// Menu: modalità, Nuova partita, Continua
// ---------------------------------------------------------------------

void UPlatformerRunSubsystem::SwitchMode(bool bBlueprints, FName FirstLevelOfMode)
{
	if (!FirstLevelOfMode.IsNone())
	{
		FirstLevel = FirstLevelOfMode;   // il game over riporta al primo livello DELLA MODALITÀ scelta
	}
	else
	{
		FirstLevel = bBlueprints ? DefaultFirstLevelBP : DefaultFirstLevelCpp;
	}
	if (bBlueprints != bBlueprintMode || !Save)
	{
		SaveProgress();                  // non perdere niente della modalità che si lascia
		bBlueprintMode = bBlueprints;
		LoadOrCreateSave();
	}
}

void UPlatformerRunSubsystem::StartMode(bool bBlueprints, FName FirstLevelOfMode)
{
	ContinueGame(bBlueprints, FirstLevelOfMode);
}

void UPlatformerRunSubsystem::NewGame(bool bBlueprints, FName FirstLevelOfMode)
{
	SwitchMode(bBlueprints, FirstLevelOfMode);

	// Salvataggio nuovo: niente abilità, niente monete. Si tengono solo i record
	// di tempo e le morti totali, che sono statistiche e non potenziamenti.
	UPlatformerSaveGame* Fresh = Cast<UPlatformerSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UPlatformerSaveGame::StaticClass()));
	if (Fresh && Save)
	{
		Fresh->BestTimes = Save->BestTimes;
		Fresh->TotalDeaths = Save->TotalDeaths;
	}
	Save = Fresh;
	if (Save)
	{
		Save->LastLevel = FirstLevel;
	}
	BankedCoins = 0;
	PendingCoins = 0;
	DeathCount = 0;
	SaveProgress();                      // da qui "Continua" riparte da questa partita nuova
	OnCoinsChanged.Broadcast(GetVisibleCoins());
	GoToLevel(FirstLevel);
}

void UPlatformerRunSubsystem::ContinueGame(bool bBlueprints, FName FirstLevelOfMode)
{
	SwitchMode(bBlueprints, FirstLevelOfMode);   // carica lo slot della modalità (se è un'altra)
	PendingCoins = 0;
	DeathCount = 0;

	FName Target = FirstLevel;
	if (Save && LevelExists(Save->LastLevel)
		&& FName(*FPackageName::GetShortName(Save->LastLevel.ToString())) != MenuLevel)
	{
		Target = Save->LastLevel;
	}
	GoToLevel(Target);
}

void UPlatformerRunSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (LoadedWorld && LoadedWorld->IsGameWorld() && LoadedWorld->GetGameInstance() == GetGameInstance())
	{
		RecordLevelReached(LoadedWorld);
	}
}

void UPlatformerRunSubsystem::RecordLevelReached(UWorld* World)
{
	if (!World)
	{
		return;
	}
	// percorso completo del livello, senza il prefisso che l'editor aggiunge in Play (UEDPIE_0_)
	const FString LevelPath = UWorld::RemovePIEPrefix(World->GetPackage()->GetName());
	const FString LevelString = FPackageName::GetShortName(LevelPath);
	const FName Level(*LevelPath);
	if (LevelString.IsEmpty() || FName(*LevelString) == MenuLevel)
	{
		return;
	}
	// Un livello _BP aperto senza passare dal menu (Play direttamente nell'editor):
	// si usa comunque lo slot giusto.
	const bool bLevelIsBP = IsBlueprintLevelName(LevelString);
	if (bLevelIsBP != bBlueprintMode)
	{
		SwitchMode(bLevelIsBP, NAME_None);
	}
	if (Save && Save->LastLevel != Level)
	{
		Save->LastLevel = Level;
		SaveProgress();
	}
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

bool UPlatformerRunSubsystem::HasAbilityByName(FName AbilityName) const
{
	return Save && !AbilityName.IsNone() && Save->UnlockedAbilities.Contains(AbilityName);
}

void UPlatformerRunSubsystem::UnlockAbilityByName(FName AbilityName)
{
	if (!Save || AbilityName.IsNone() || HasAbilityByName(AbilityName))
	{
		return;
	}
	Save->UnlockedAbilities.Add(AbilityName);
	SaveProgress();
	// se il nome è una delle abilità C++, avvisa anche chi ascolta l'evento
	for (EPlatformerAbility A : { EPlatformerAbility::DoubleJump, EPlatformerAbility::WallRun,
		EPlatformerAbility::Jetpack, EPlatformerAbility::CrateSpawner })
	{
		if (AbilityToName(A) == AbilityName)
		{
			OnAbilityUnlocked.Broadcast(A);
		}
	}
}

void UPlatformerRunSubsystem::PushMessage(const FString& Message, FLinearColor Color)
{
	OnMessage.Broadcast(Message, Color);
}
