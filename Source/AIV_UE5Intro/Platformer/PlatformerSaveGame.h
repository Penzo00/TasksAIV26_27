// GD16 - Il salvataggio: una classe Save Game con i dati da tenere fra una
// sessione e l'altra (monete, abilità sbloccate, record, morti).
// È la stessa idea della lezione (Create Save Game Object, Save Game to Slot,
// Load Game from Slot), scritta in C++.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PlatformerSaveGame.generated.h"

UCLASS()
class AIV_UE5INTRO_API UPlatformerSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Monete messe "in banca" (cioè raccolte in un livello poi completato). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Salvataggio")
	int32 Coins = 0;

	/** Abilità sbloccate (per esempio DoubleJump). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Salvataggio")
	TArray<FName> UnlockedAbilities;

	/** Miglior tempo per ogni livello, in secondi (nome della mappa -> tempo). */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Salvataggio")
	TMap<FName, float> BestTimes;

	/** Morti totali da quando esiste il salvataggio. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Salvataggio")
	int32 TotalDeaths = 0;
};
