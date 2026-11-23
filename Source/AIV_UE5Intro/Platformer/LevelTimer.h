// GD13 - Il livello a tempo: se non si arriva al traguardo entro TimeLimit
// secondi, il livello riparte. Il tempo rimasto è scritto a schermo.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelTimer.generated.h"

class UTextRenderComponent;

UCLASS()
class AIV_UE5INTRO_API ALevelTimer : public AActor
{
	GENERATED_BODY()

public:
	ALevelTimer();

	/** Secondi a disposizione per finire il livello. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer", meta = (ClampMin = "1.0"))
	float TimeLimit = 60.f;

	/** Sotto questa soglia il tempo diventa rosso. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer", meta = (ClampMin = "0.0"))
	float WarningThreshold = 10.f;

	/** Scrive il tempo anche come messaggio di debug in alto a sinistra (l'HUD lo mostra già). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	bool bShowOnScreen = false;

	/** Scrive il tempo anche nel mondo, sulla scritta dell'actor (utile se lo si piazza in vista). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	bool bShowInWorld = true;

	UFUNCTION(BlueprintPure, Category = "Timer")
	float GetTimeLeft() const { return TimeLeft; }

	/** Ferma il conto alla rovescia (per esempio al traguardo). */
	UFUNCTION(BlueprintCallable, Category = "Timer")
	void StopTimer() { bRunning = false; }

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Timer")
	TObjectPtr<UTextRenderComponent> WorldText;

private:
	float TimeLeft = 0.f;
	bool bRunning = false;
};
