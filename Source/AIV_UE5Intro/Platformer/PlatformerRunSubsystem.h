// GD13/GD16 - Lo stato della "partita" che sopravvive ai cambi di livello.
// È un sottosistema della Game Instance: esiste finché il gioco è aperto,
// come la Game Instance della lezione, ma non obbliga a cambiare GI_Platform.
// Tiene: morti, monete (con il "conto" del livello in corso), abilità
// sbloccate, tempi migliori. Salva e carica tutto con una Save Game.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlatformerRunSubsystem.generated.h"

class UPlatformerSaveGame;

/** Le abilità che si possono sbloccare nei livelli. */
UENUM(BlueprintType)
enum class EPlatformerAbility : uint8
{
	DoubleJump   UMETA(DisplayName = "Doppio salto"),
	WallRun      UMETA(DisplayName = "Wall run e wall jump"),
	Jetpack      UMETA(DisplayName = "Jetpack"),
	CrateSpawner UMETA(DisplayName = "Generatore di casse")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlatformerAbilityUnlocked, EPlatformerAbility, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlatformerCoinsChanged, int32, VisibleCoins);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlatformerMessage, const FString&, Message, FLinearColor, Color);

UCLASS()
class AIV_UE5INTRO_API UPlatformerRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Comodità: trova il sottosistema partendo da qualunque oggetto nel mondo. */
	static UPlatformerRunSubsystem* Get(const UObject* WorldContext);

	// ---------------------------------------------------------------- livelli

	/** Conta una morte e fa ripartire il livello corrente dopo Delay secondi. */
	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void KillPlayerAndRestart(float Delay = 0.f);

	/** Riapre il livello corrente (senza contare una morte). */
	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void RestartCurrentLevel();

	/** Apre un altro livello (Open Level). */
	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void GoToLevel(FName LevelName);

	/** Chiamato dal traguardo: "incassa" le monete del livello, registra il tempo e salva. */
	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void CompleteLevel();

	/** Lo chiama la GameMode a ogni inizio livello: azzera il conto del livello e il cronometro. */
	void NotifyLevelStarted(UWorld* World);

	/** Secondi passati dall'inizio del livello (si ferma al traguardo). */
	UFUNCTION(BlueprintPure, Category = "Platformer")
	float GetLevelElapsedTime() const;

	/** Miglior tempo del livello corrente (0 = nessuno). */
	UFUNCTION(BlueprintPure, Category = "Platformer")
	float GetBestTimeForCurrentLevel() const;

	UFUNCTION(BlueprintPure, Category = "Platformer")
	int32 GetDeathCount() const { return DeathCount; }

	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void ResetDeathCount() { DeathCount = 0; }

	// ---------------------------------------------------------------- vite

	/** Vite a ogni inizio livello. Si perde una vita cadendo (si torna all'ultimo posto sicuro). */
	static constexpr int32 MaxLives = 3;

	/** Il primo livello: con zero vite si ricomincia da qui. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platformer|Vite")
	FName FirstLevel = FName(TEXT("L_GD13_Livello1"));

	/** Se vero, il game over toglie anche le abilità sbloccate: si ricomincia davvero da capo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platformer|Vite")
	bool bGameOverResetsAbilities = true;

	UFUNCTION(BlueprintPure, Category = "Platformer|Vite")
	int32 GetLives() const { return Lives; }

	UFUNCTION(BlueprintPure, Category = "Platformer|Vite")
	int32 GetMaxLives() const { return MaxLives; }

	/** Toglie una vita. Vero = ne restano (si torna al posto sicuro); falso = game over (già avviato). */
	UFUNCTION(BlueprintCallable, Category = "Platformer|Vite")
	bool LoseLife();

	/** Zero vite: messaggio, e si riparte dal primo livello. */
	UFUNCTION(BlueprintCallable, Category = "Platformer|Vite")
	void GameOver();

	// ---------------------------------------------------------------- menu e modalità

	/** Il livello del menu di scelta della modalità (C++ o Blueprints). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platformer|Menu")
	FName MenuLevel = FName(TEXT("L_GD13_Menu"));

	/** Vero se si sta giocando la versione Blueprint dei livelli (scelta nel menu). */
	UFUNCTION(BlueprintPure, Category = "Platformer|Menu")
	bool IsBlueprintMode() const { return bBlueprintMode; }

	/** Chiamato dal menu: ricorda la modalità, imposta il primo livello e lo apre. */
	UFUNCTION(BlueprintCallable, Category = "Platformer|Menu")
	void StartMode(bool bBlueprints, FName FirstLevelOfMode);

	/** Torna al menu di scelta della modalità (tasto M nei livelli C++). */
	UFUNCTION(BlueprintCallable, Category = "Platformer|Menu")
	void GoToMenu();

	// ---------------------------------------------------------------- monete

	/** Aggiunge monete al conto del livello in corso (diventano tue al traguardo). */
	UFUNCTION(BlueprintCallable, Category = "Platformer|Monete")
	void AddPendingCoins(int32 Amount);

	/** Spende monete: prima quelle del livello, poi quelle in banca. Falso se non bastano. */
	UFUNCTION(BlueprintCallable, Category = "Platformer|Monete")
	bool SpendCoins(int32 Amount);

	/** Monete in banca + monete raccolte nel livello in corso. */
	UFUNCTION(BlueprintPure, Category = "Platformer|Monete")
	int32 GetVisibleCoins() const { return BankedCoins + PendingCoins; }

	UFUNCTION(BlueprintPure, Category = "Platformer|Monete")
	int32 GetPendingCoins() const { return PendingCoins; }

	// ---------------------------------------------------------------- abilità

	UFUNCTION(BlueprintPure, Category = "Platformer|Abilita")
	bool HasAbility(EPlatformerAbility Ability) const;

	/** Sblocca un'abilità e salva subito (non si perde morendo). */
	UFUNCTION(BlueprintCallable, Category = "Platformer|Abilita")
	void UnlockAbility(EPlatformerAbility Ability);

	// ---------------------------------------------------------------- salvataggio

	UFUNCTION(BlueprintCallable, Category = "Platformer|Salvataggio")
	void SaveProgress();

	/** Cancella il salvataggio e riparte da zero (comando da console: AIVResetProgress). */
	UFUNCTION(BlueprintCallable, Category = "Platformer|Salvataggio")
	void ResetProgress();

	/** Messaggio a schermo per l'HUD (per esempio "Doppio salto sbloccato!"). */
	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void PushMessage(const FString& Message, FLinearColor Color = FLinearColor::White);

	UPROPERTY(BlueprintAssignable, Category = "Platformer")
	FOnPlatformerAbilityUnlocked OnAbilityUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Platformer")
	FOnPlatformerCoinsChanged OnCoinsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Platformer")
	FOnPlatformerMessage OnMessage;

	/** Nome dello slot di salvataggio. */
	static const FString SaveSlotName;

	static FName AbilityToName(EPlatformerAbility Ability);

private:
	void LoadOrCreateSave();
	FName CurrentLevelKey() const;

	UPROPERTY()
	TObjectPtr<UPlatformerSaveGame> Save;

	void OpenFirstLevel();

	int32 DeathCount = 0;
	int32 Lives = MaxLives;
	int32 BankedCoins = 0;
	int32 PendingCoins = 0;
	double LevelStartTime = 0.0;
	double LevelEndTime = -1.0;
	bool bRestartPending = false;
	bool bBlueprintMode = false;
	FTimerHandle RestartHandle;
};
