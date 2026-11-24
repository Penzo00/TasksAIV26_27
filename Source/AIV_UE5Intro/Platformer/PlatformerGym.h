// Gli actor "da gym" chiesti per il task:
//   ASpawnedCrate          la cassa del generatore (UPlatformerCrateSpawnerComponent)
//   AScaleByDistanceActor  più il giocatore è lontano, più diventa grande
//   AModelCyclerActor      cambia modello (mesh) ogni secondo
//   ARandomPresenceActor   a ogni Play c'è solo il 35% delle volte
// Tutti hanno una scritta sopra (opzionale) e i parametri nei Details.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlatformerGym.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;

/** La cassa generata dal giocatore: un cubo solido su cui si può salire. */
UCLASS()
class AIV_UE5INTRO_API ASpawnedCrate : public AActor
{
	GENERATED_BODY()

public:
	ASpawnedCrate();

	/** Dimensione (lato in cm) e materiale. */
	UFUNCTION(BlueprintCallable, Category = "Cassa")
	void SetCrate(float Size, UMaterialInterface* Material);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cassa")
	TObjectPtr<UStaticMeshComponent> Mesh;
};

/**
 * Più il giocatore è lontano, più diventa grande (e viceversa).
 * Scala = lerp(MinScale, MaxScale) secondo la distanza fra NearDistance e
 * FarDistance. Usato come "faro" che si vede sempre e come piattaforma che
 * si restringe mentre ci si avvicina.
 */
UCLASS()
class AIV_UE5INTRO_API AScaleByDistanceActor : public AActor
{
	GENERATED_BODY()

public:
	AScaleByDistanceActor();

	/** La mesh (cubo, sfera...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza")
	TObjectPtr<UStaticMesh> ShapeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza")
	TObjectPtr<UMaterialInterface> Material;

	/** Dimensione a scala 1, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza")
	FVector BaseSize = FVector(300.f, 300.f, 30.f);

	/** Scala quando il giocatore è vicino (entro NearDistance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza", meta = (ClampMin = "0.01"))
	float MinScale = 0.4f;

	/** Scala quando il giocatore è lontano (oltre FarDistance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza", meta = (ClampMin = "0.01"))
	float MaxScale = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza", meta = (ClampMin = "0.0"))
	float NearDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza", meta = (ClampMin = "1.0"))
	float FarDistance = 2000.f;

	/** Se falso, cambia solo larghezza e profondità (per le piattaforme: lo spessore resta quello). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza")
	bool bScaleHeight = true;

	/** Quanto in fretta segue la scala voluta (0 = subito). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza", meta = (ClampMin = "0.0"))
	float InterpSpeed = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scala con la distanza")
	FText Label;

	UFUNCTION(BlueprintPure, Category = "Scala con la distanza")
	float GetCurrentScale() const { return CurrentScale; }

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Scala con la distanza")
	void ApplySettings();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scala con la distanza")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scala con la distanza")
	TObjectPtr<UTextRenderComponent> LabelText;

private:
	void SetScaleNow(float Scale);
	float CurrentScale = 1.f;
};

/** Cambia modello ogni Interval secondi (di default: cubo, cilindro, sfera, cono). */
UCLASS()
class AIV_UE5INTRO_API AModelCyclerActor : public AActor
{
	GENERATED_BODY()

public:
	AModelCyclerActor();

	/** I modelli, in ordine. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cambia modello")
	TArray<TObjectPtr<UStaticMesh>> Models;

	/** Materiali (uno per modello, oppure uno solo per tutti; vuoto = quelli della mesh). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cambia modello")
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	/** Secondi fra un modello e l'altro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cambia modello", meta = (ClampMin = "0.05"))
	float Interval = 1.f;

	/** Dimensione di ogni modello (le forme dell'engine sono di 100 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cambia modello")
	FVector Size = FVector(200.f, 200.f, 200.f);

	/** Se vero, ci si può salire sopra (è una piattaforma). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cambia modello")
	bool bCollide = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cambia modello")
	FText Label;

	/** Passa subito al modello successivo. */
	UFUNCTION(BlueprintCallable, Category = "Cambia modello")
	void NextModel();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cambia modello")
	void ApplySettings();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cambia modello")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cambia modello")
	TObjectPtr<UTextRenderComponent> LabelText;

private:
	void ShowModel(int32 Index);
	int32 CurrentIndex = 0;
	FTimerHandle CycleHandle;
};

/**
 * Un oggetto che a ogni Play c'è solo con probabilità SpawnChance (35%):
 * nelle altre partite si distrugge in BeginPlay, come se non ci fosse mai
 * stato. Nel livello 1 è un ponte "fortunato" che accorcia la strada.
 */
UCLASS()
class AIV_UE5INTRO_API ARandomPresenceActor : public AActor
{
	GENERATED_BODY()

public:
	ARandomPresenceActor();

	/** Probabilità di esserci (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presenza casuale", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnChance = 0.35f;

	/** Dimensione in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presenza casuale")
	FVector Size = FVector(1000.f, 200.f, 30.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presenza casuale")
	TObjectPtr<UStaticMesh> ShapeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presenza casuale")
	TObjectPtr<UMaterialInterface> Material;

	/** Se vero, un messaggio a schermo dice se questa volta c'è. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presenza casuale")
	bool bAnnounce = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presenza casuale", meta = (EditCondition = "bAnnounce"))
	FString PresentMessage = TEXT("Fortuna! L'oggetto fortunato questa volta c'e' (35%)");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presenza casuale", meta = (EditCondition = "bAnnounce"))
	FString AbsentMessage = TEXT("L'oggetto fortunato questa volta non c'e'...");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presenza casuale")
	FText Label;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Presenza casuale")
	void ApplySettings();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presenza casuale")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presenza casuale")
	TObjectPtr<UTextRenderComponent> LabelText;
};
