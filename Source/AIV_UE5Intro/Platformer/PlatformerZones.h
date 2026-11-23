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

	/** Moltiplica l'attrito a terra (Ground Friction). Meno di 1 = si scivola: il ghiaccio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modificatore", meta = (ClampMin = "0.0"))
	float GroundFrictionMultiplier = 1.f;

	/** Moltiplica la frenata (Braking Deceleration Walking). Meno di 1 = ci si ferma tardi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modificatore", meta = (ClampMin = "0.0"))
	float BrakingMultiplier = 1.f;

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
	float SavedFriction = 8.f;
	float SavedBraking = 2048.f;
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

class AActivable;

/** Teletrasporto (GD7): porta il personaggio in un altro punto del livello. */
UCLASS()
class AIV_UE5INTRO_API ATeleportZone : public APlatformerZone
{
	GENERATED_BODY()

public:
	/** Punto di arrivo, relativo alla zona (si trascina nel viewport con il rombo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teletrasporto", meta = (MakeEditWidget = "true"))
	FVector Destination = FVector(500.f, 0.f, 100.f);

	/** Se vero, all'arrivo si azzera la velocità. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teletrasporto")
	bool bStopVelocity = true;

protected:
	virtual void OnCharacterEntered(ACharacter* Character) override;
};

/**
 * Il "negozio" (UE8, BP_VolumeShop): entrando con abbastanza monete le si
 * spende e si attivano gli attivabili collegati (per esempio un cancello).
 */
UCLASS()
class AIV_UE5INTRO_API AShopZone : public APlatformerZone
{
	GENERATED_BODY()

public:
	/** Monete richieste. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Negozio", meta = (ClampMin = "0"))
	int32 Cost = 5;

	/** Cosa si attiva pagando (porte, piattaforme, luci...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Negozio")
	TArray<TObjectPtr<AActivable>> Targets;

	/** Se vero, si paga una volta sola. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Negozio")
	bool bOnlyOnce = true;

protected:
	virtual void BeginPlay() override;
	virtual void OnCharacterEntered(ACharacter* Character) override;

private:
	bool bPaid = false;
};
