// GD13/GD14 - GameMode dei livelli dei task: il personaggio è BP_Bot2.
// Quando Bot2 nasce gli aggiunge i componenti (vita, abilità, jetpack,
// torcia, posto sicuro e vite, generatore di casse)
// (così non serve modificare il Blueprint), usa l'HUD del platformer e
// avvisa la Game Instance che il livello è cominciato.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PlatformerGameMode.generated.h"

UCLASS()
class AIV_UE5INTRO_API APlatformerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APlatformerGameMode();

	/** Se vero, il doppio salto è disponibile anche senza averlo raccolto (per provare i livelli). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platformer")
	bool bDoubleJumpForTesting = false;

	/** Se vero, TUTTE le abilità (doppio salto, jetpack, casse) sono disponibili senza raccoglierle (per i test). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platformer")
	bool bAllAbilitiesForTesting = false;

	virtual void StartPlay() override;
	virtual void SetPlayerDefaults(APawn* PlayerPawn) override;

	/** Comando da console (tasto ò / backtick): cancella il salvataggio. */
	UFUNCTION(Exec)
	void AIVResetProgress();

	/** Comando da console: sblocca tutte le abilità. */
	UFUNCTION(Exec)
	void AIVUnlockAll();
};
