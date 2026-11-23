// Pezzi per "vestire" i livelli, dalle lezioni di environment:
//   ASplineFence      UE16: la staccionata lungo una spline, "per lunghezza"
//                     (un paletto ogni N cm) con le traverse fra i paletti
//   AScatterArea      UE15: un foliage "a mano": sparge istanze di una mesh
//                     in un'area, appoggiandole al terreno con un trace
//   AFlickeringLight  UE13/GD14: la luce del fuoco o di una candela, che
//                     tremola con un rumore di Perlin (luce "viva")

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelDressing.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USplineComponent;
class UStaticMesh;
class UMaterialInterface;

UCLASS()
class AIV_UE5INTRO_API ASplineFence : public AActor
{
	GENERATED_BODY()

public:
	ASplineFence();

	/** Mesh dei paletti (vuota = cilindro). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Staccionata")
	TObjectPtr<UStaticMesh> PostMesh;

	/** Un paletto ogni Spacing cm lungo la spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Staccionata", meta = (ClampMin = "20.0"))
	float Spacing = 200.f;

	/** Dimensioni di un paletto (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Staccionata")
	FVector PostSize = FVector(14.f, 14.f, 110.f);

	/** Quante traverse fra un paletto e l'altro (0 = nessuna). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Staccionata", meta = (ClampMin = "0", ClampMax = "5"))
	int32 Rails = 2;

	/** Spessore delle traverse (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Staccionata")
	float RailThickness = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Staccionata")
	TObjectPtr<UMaterialInterface> Material;

	/** Se vero, la staccionata blocca il personaggio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Staccionata")
	bool bCollide = true;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Staccionata")
	void RebuildFence();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Staccionata")
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Staccionata")
	TObjectPtr<UInstancedStaticMeshComponent> Posts;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Staccionata")
	TObjectPtr<UInstancedStaticMeshComponent> RailInstances;
};

UCLASS()
class AIV_UE5INTRO_API AScatterArea : public AActor
{
	GENERATED_BODY()

public:
	AScatterArea();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia")
	TObjectPtr<UStaticMesh> Mesh;

	/** Metà delle dimensioni dell'area (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia")
	FVector AreaExtent = FVector(1000.f, 1000.f, 500.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia", meta = (ClampMin = "0", ClampMax = "2000"))
	int32 Count = 30;

	/** Seme del caso: stesso seme = stessa disposizione. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia")
	int32 Seed = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia")
	float MinScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia")
	float MaxScale = 1.3f;

	/** Distanza minima fra due istanze (cm), come il raggio del foliage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia", meta = (ClampMin = "0.0"))
	float MinDistance = 150.f;

	/** Se vero, ogni istanza è appoggiata al terreno con un trace verso il basso. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia")
	bool bAlignToGround = true;

	/** Se vero, le istanze hanno collisione (gli alberi sì, l'erba no). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia")
	bool bCollide = false;

	/** Oltre questa distanza le istanze spariscono (0 = mai). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sparpaglia", meta = (ClampMin = "0.0"))
	float CullDistance = 0.f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Sparpaglia")
	void Rebuild();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sparpaglia")
	TObjectPtr<UBoxComponent> Area;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sparpaglia")
	TObjectPtr<UInstancedStaticMeshComponent> Instances;
};

UCLASS()
class AIV_UE5INTRO_API AFlickeringLight : public AActor
{
	GENERATED_BODY()

public:
	AFlickeringLight();

	/** Intensità media (candele). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce", meta = (ClampMin = "0.0"))
	float BaseIntensity = 60.f;

	/** Quanto varia l'intensità (0 = ferma, 0.5 = ±50%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerAmount = 0.25f;

	/** Velocità del tremolio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce", meta = (ClampMin = "0.0"))
	float FlickerSpeed = 6.f;

	/** Quanto si sposta la sorgente (cm): le ombre "respirano" come con una fiamma vera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce", meta = (ClampMin = "0.0"))
	float PositionJitter = 4.f;

	/** Temperatura colore (K): 1900 fuoco, 2700 lampadina calda, 6500 luce fredda. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce", meta = (ClampMin = "1000.0", ClampMax = "15000.0"))
	float Temperature = 1900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce", meta = (ClampMin = "1.0"))
	float AttenuationRadius = 900.f;

	/** Raggio della sorgente (cm): più grande = ombre più morbide. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce", meta = (ClampMin = "0.0"))
	float SourceRadius = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce")
	bool bCastShadows = true;

	/** Attori con un materiale emissivo da far tremolare insieme alla luce (braci, fiamme). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce")
	TArray<TObjectPtr<AActor>> GlowActors;

	/** Nome del parametro scalare del materiale emissivo da animare. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce")
	FName GlowParameter = FName(TEXT("Intensita"));

	/** Valore del parametro quando la luce è all'intensità media. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce")
	float GlowBaseValue = 8.f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Luce")
	void ApplyLightSettings();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Luce")
	TObjectPtr<UPointLightComponent> Light;

private:
	float Clock = 0.f;
	float Seed = 0.f;
};
