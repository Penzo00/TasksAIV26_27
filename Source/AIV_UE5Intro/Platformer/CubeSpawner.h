// UE5/UE6 - Lo spawn e la matrice di cubi: due For Loop (righe e colonne)
// e, per ogni cella, un Line Trace verso il basso che trova il pavimento;
// lì si fa Spawn Actor From Class di un cubo con la fisica.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubeSpawner.generated.h"

class UBillboardComponent;
class UStaticMeshComponent;
class UMaterialInterface;

/** Il cubo che si spawna (il BP_Cube della lezione): una scatola con la fisica. */
UCLASS()
class AIV_UE5INTRO_API APhysicsCube : public AActor
{
	GENERATED_BODY()

public:
	APhysicsCube();

	UStaticMeshComponent* GetMesh() const { return Mesh; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cubo")
	TObjectPtr<UStaticMeshComponent> Mesh;
};

UCLASS()
class AIV_UE5INTRO_API ACubeSpawner : public AActor
{
	GENERATED_BODY()

public:
	ACubeSpawner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "1", ClampMax = "50"))
	int32 Rows = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "1", ClampMax = "50"))
	int32 Columns = 4;

	/** Quanti cubi uno sopra l'altro in ogni cella. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "1", ClampMax = "10"))
	int32 CubeLayers = 1;

	/** Lato di un cubo (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "10.0"))
	float CubeSize = 60.f;

	/** Distanza fra i centri dei cubi (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "10.0"))
	float Spacing = 70.f;

	/** Se vero, i cubi hanno la fisica (si spingono e cadono). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bSimulatePhysics = true;

	/** Massa di ogni cubo (kg): leggera = il personaggio li spinge facilmente. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner", meta = (ClampMin = "0.1"))
	float CubeMass = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	/** Disegna in editor le linee dei trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	bool bDebugTraces = false;

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SpawnCubes();

protected:
	virtual void BeginPlay() override;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Icon;
#endif
};
