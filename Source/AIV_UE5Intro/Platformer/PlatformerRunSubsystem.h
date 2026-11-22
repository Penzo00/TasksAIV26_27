// GD13 - Lo stato della "partita" che sopravvive ai cambi di livello.
// È un sottosistema della Game Instance: esiste finché il gioco è aperto,
// come la Game Instance della lezione, ma non obbliga a cambiare GI_Platform.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlatformerRunSubsystem.generated.h"

UCLASS()
class AIV_UE5INTRO_API UPlatformerRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Comodità: trova il sottosistema partendo da qualunque oggetto nel mondo. */
	static UPlatformerRunSubsystem* Get(const UObject* WorldContext);

	/** Conta una morte e fa ripartire il livello corrente dopo Delay secondi. */
	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void KillPlayerAndRestart(float Delay = 0.f);

	/** Riapre il livello corrente (senza contare una morte). */
	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void RestartCurrentLevel();

	/** Apre un altro livello (Open Level). */
	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void GoToLevel(FName LevelName);

	UFUNCTION(BlueprintPure, Category = "Platformer")
	int32 GetDeathCount() const { return DeathCount; }

	UFUNCTION(BlueprintCallable, Category = "Platformer")
	void ResetDeathCount() { DeathCount = 0; }

private:
	int32 DeathCount = 0;
	bool bRestartPending = false;
	FTimerHandle RestartHandle;
};
