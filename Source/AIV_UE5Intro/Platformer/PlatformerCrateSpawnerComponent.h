// Il generatore di casse: premendo F (o il tasto sinistro del pad) compare
// una cassa davanti al personaggio, appoggiata al pavimento (o sopra
// un'altra cassa: così si impilano). Si possono generare 3 casse; finite
// le cariche bisogna aspettare 5 secondi e tornano tutte e 3.
// Si sblocca con l'oggetto "GENERATORE DI CASSE".
//
// In Blueprint: un intero Cariche; al tasto, se Cariche > 0 -> Line Trace
// verso il basso davanti al personaggio -> Spawn Actor -> Cariche - 1;
// se Cariche == 0 -> Set Timer by Event (5 s) che rimette Cariche = 3.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "PlatformerCrateSpawnerComponent.generated.h"

class ACharacter;
class ASpawnedCrate;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;
class UMaterialInterface;

UCLASS(ClassGroup = (Platformer), meta = (BlueprintSpawnableComponent))
class AIV_UE5INTRO_API UPlatformerCrateSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlatformerCrateSpawnerComponent();

	/** Se vero, funziona anche senza averlo raccolto (per i test). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse")
	bool bAlwaysUnlocked = false;

	/** Quante casse si possono generare prima della ricarica. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse", meta = (ClampMin = "1"))
	int32 MaxCharges = 3;

	/** Secondi di attesa quando le cariche sono finite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse", meta = (ClampMin = "0.1"))
	float RechargeTime = 5.f;

	/** Distanza davanti al personaggio dove compare la cassa (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse", meta = (ClampMin = "50.0"))
	float SpawnDistance = 130.f;

	/** Lato della cassa (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse", meta = (ClampMin = "10.0"))
	float CrateSize = 80.f;

	/** Dopo quanti secondi una cassa sparisce (0 = mai). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse", meta = (ClampMin = "0.0"))
	float CrateLifetime = 25.f;

	/** Quante casse possono esistere insieme: oltre, sparisce la più vecchia. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse", meta = (ClampMin = "1"))
	int32 MaxAliveCrates = 6;

	/** La classe della cassa (si può cambiare con un Blueprint figlio di ASpawnedCrate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse")
	TSubclassOf<ASpawnedCrate> CrateClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse")
	TObjectPtr<UMaterialInterface> CrateMaterial;

	/** I tasti per generare una cassa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casse|Input")
	TArray<FKey> SpawnKeys;

	UFUNCTION(BlueprintPure, Category = "Casse")
	bool IsUnlocked() const;

	UFUNCTION(BlueprintPure, Category = "Casse")
	int32 GetCharges() const { return Charges; }

	/** Secondi alla fine della ricarica (0 se non si sta ricaricando). */
	UFUNCTION(BlueprintPure, Category = "Casse")
	float GetRechargeTimeLeft() const;

	/** Genera una cassa (lo chiama il tasto). Falso se non si può. */
	UFUNCTION(BlueprintCallable, Category = "Casse")
	bool SpawnCrate();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void TryBindInput();
	void HandleSpawnPressed();
	void Recharge();

	UPROPERTY()
	TObjectPtr<ACharacter> Character;

	UPROPERTY()
	TObjectPtr<UInputAction> SpawnAction;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> SpawnContext;

	TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
	TArray<TWeakObjectPtr<ASpawnedCrate>> Alive;
	bool bContextAdded = false;
	int32 Charges = 3;
	FTimerHandle RechargeHandle;
};
