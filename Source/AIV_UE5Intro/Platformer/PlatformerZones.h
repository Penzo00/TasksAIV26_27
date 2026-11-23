// GD13 - Le zone concrete: ognuna è figlia di APlatformerZone e ridefinisce
// soltanto che cosa succede quando il personaggio entra o esce.

#pragma once

#include "CoreMinimal.h"
#include "PlatformerZone.h"
#include "PlatformerZones.generated.h"

class ACharacter;

/** Trappola: toccarla fa ripartire il livello (buchi, spuntoni, acqua...). */
UCLASS()
class AIV_UE5INTRO_API AKillZone : public APlatformerZone
{
	GENERATED_BODY()

public:
	/** Secondi prima che il livello riparta. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trappola", meta = (ClampMin = "0.0"))
	float RestartDelay = 0.3f;

protected:
	virtual void OnCharacterEntered(ACharacter* Character) override;
};

/** Area di danno: toglie vita finché si resta dentro (usa il componente vita). */
UCLASS()
class AIV_UE5INTRO_API ADamageZone : public APlatformerZone
{
	GENERATED_BODY()

public:
	/** Danno inflitto a ogni "colpo". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danno", meta = (ClampMin = "0.0"))
	float DamagePerHit = 20.f;

	/** Secondi tra un colpo e l'altro, finché si resta nella zona. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Danno", meta = (ClampMin = "0.05"))
	float HitInterval = 0.5f;

protected:
	virtual void OnCharacterEntered(ACharacter* Character) override;
	virtual void OnCharacterExited(ACharacter* Character) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void DealDamage();

	TWeakObjectPtr<ACharacter> Victim;
	FTimerHandle DamageHandle;
};

/** Zona che cambia il personaggio: più veloce, salta più in alto, gravità diversa. */
UCLASS()
class AIV_UE5INTRO_API AModifierZone : public APlatformerZone
{
	GENERATED_BODY()

public:
	/** Moltiplica la velocità di camminata (1 = invariata). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modificatore", meta = (ClampMin = "0.1"))
	float WalkSpeedMultiplier = 1.f;

	/** Moltiplica la forza del salto (1 = invariata). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modificatore", meta = (ClampMin = "0.1"))
	float JumpMultiplier = 1.f;

	/** Moltiplica la gravità (meno di 1 = salti "lunari"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modificatore", meta = (ClampMin = "0.05"))
	float GravityMultiplier = 1.f;

	/**
	 * Quanto dura l'effetto dopo essere USCITI dalla zona, in secondi.
	 * 0 = finisce appena si esce. Negativo = dura per tutto il livello.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modificatore")
	float DurationAfterExit = 3.f;

protected:
	virtual void OnCharacterEntered(ACharacter* Character) override;
	virtual void OnCharacterExited(ACharacter* Character) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Restore();

	TWeakObjectPtr<ACharacter> Affected;
	float SavedWalkSpeed = 0.f;
	float SavedJumpZ = 0.f;
	float SavedGravity = 1.f;
	bool bApplied = false;
	FTimerHandle RestoreHandle;
};

/** Traguardo: porta al livello successivo (Open Level). */
UCLASS()
class AIV_UE5INTRO_API ALevelGoal : public APlatformerZone
{
	GENERATED_BODY()

public:
	/** Nome della mappa da aprire, per esempio L_GD13_Livello2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traguardo")
	FName NextLevel;

	/** Messaggio a schermo quando si arriva. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traguardo")
	FString Message = TEXT("Livello completato!");

	/** Secondi prima del cambio di livello. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traguardo", meta = (ClampMin = "0.0"))
	float Delay = 1.f;

	/** Se vero, azzera il contatore delle morti (da usare sull'ultimo traguardo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traguardo")
	bool bResetDeathCount = false;

protected:
	virtual void OnCharacterEntered(ACharacter* Character) override;

private:
	void OpenNext();

	bool bReached = false;
	FTimerHandle GoalHandle;
};
