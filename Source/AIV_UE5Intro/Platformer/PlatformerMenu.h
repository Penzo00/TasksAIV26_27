// Il menu iniziale, in due passi.
//
// 1) Modalità: i livelli fatti in C++ (L_GD13_Livello1..3, si gioca con Bot2)
//    o la loro copia rifatta in Blueprint (L_GD13_Livello1_BP..3_BP, si gioca
//    con BP_Hero). Tasti: 1 o C = C++, 2 o B = Blueprints.
// 2) Partita: "Nuova partita" (dal livello 1, senza potenziamenti) oppure
//    "Continua" (dall'ultimo livello raggiunto, con potenziamenti e monete).
//    Sotto ogni pulsante un testo spiega cosa succede. "Continua" è spento se
//    la modalità non ha ancora un salvataggio. Tasti: 1 o N = Nuova partita,
//    2 o C = Continua, Esc / Backspace / pad B = indietro.
//
// In entrambi i passi: clic del mouse (hit box dell'HUD), frecce o pad
// sinistra/destra + Invio / A. Il salvataggio vero e proprio lo fa il C++
// (UPlatformerRunSubsystem: NewGame, ContinueGame, uno slot per modalità).
// Tutto è disegnato con il Canvas, come APlatformerHUD.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "PlatformerMenu.generated.h"

UCLASS()
class AIV_UE5INTRO_API APlatformerMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APlatformerMenuGameMode();

	virtual void StartPlay() override;
};

UCLASS()
class AIV_UE5INTRO_API APlatformerMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** Primo livello della versione C++. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName CppFirstLevel = FName(TEXT("L_GD13_Livello1"));

	/** Primo livello della versione Blueprint (nome consigliato: quello C++ con _BP in fondo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName BlueprintFirstLevel = FName(TEXT("L_GD13_Livello1_BP"));

	/** Cartella dei livelli, per controllare che il livello Blueprint esista. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FString MapsFolder = TEXT("/Game/Maps/");

	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;
	virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;
	virtual void NotifyHitBoxEndCursorOver(FName BoxName) override;

	/** Passo 1: sceglie la modalità (0 = C++, 1 = Blueprints) e mostra Nuova partita / Continua. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void Choose(int32 Option);

	/** Passo 2: avvia la partita. Falso = Nuova partita, vero = Continua. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartGame(bool bContinue);

	/** Dal passo 2 torna alla scelta della modalità. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void Back();

private:
	bool BlueprintLevelExists() const;
	void RefreshSaveInfo();
	void HandleKeys();
	void ShowNotice(const FString& Text);
	void DrawModeStep(float SW, float SH, float UIScale);
	void DrawGameStep(float SW, float SH, float UIScale);
	void DrawButton(int32 Index, FName BoxName, bool bEnabled, const FString& Title, const TArray<FString>& Lines,
		float X, float Y, float W, float H, const FLinearColor& Accent);
	void DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, bool bLarge);

	/** 0 = scelta della modalità, 1 = Nuova partita / Continua. */
	int32 Step = 0;
	int32 Selected = 0;
	bool bModeBlueprints = false;
	bool bChosen = false;
	FString Notice;
	float NoticeTime = -100.f;

	// riassunto del salvataggio della modalità scelta (letto una volta entrando nel passo 2)
	bool bHasSave = false;
	FName SavedLevel;
	int32 SavedCoins = 0;
	TArray<FName> SavedAbilities;
};
