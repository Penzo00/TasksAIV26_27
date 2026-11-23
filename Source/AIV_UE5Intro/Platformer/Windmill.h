// UE4 - Il mulino: le pale girano sull'Event Tick (Combine Rotators, con il
// Delta Time), la testa "guarda" il player con Find Look at Rotation e un
// Clamp. La gerarchia dei componenti (torre > testa > mozzo > pale) fa sì
// che ruotando il padre ruotino anche i figli.
// Con bBladesCollide le pale diventano un ostacolo che spinge.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Windmill.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class AIV_UE5INTRO_API AWindmill : public AActor
{
	GENERATED_BODY()

public:
	AWindmill();

	/** Gradi al secondo delle pale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mulino")
	float BladeSpeed = 60.f;

	/** Se vero, la testa si gira verso il player (entro YawLimit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mulino")
	bool bLookAtPlayer = true;

	/** Quanto può girarsi la testa a destra e a sinistra (gradi, il Clamp). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mulino", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float YawLimit = 60.f;

	/** Altezza della torre (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mulino", meta = (ClampMin = "100.0"))
	float TowerHeight = 900.f;

	/** Lunghezza di una pala (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mulino", meta = (ClampMin = "50.0"))
	float BladeLength = 450.f;

	/** Se vero, le pale hanno collisione (ostacolo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mulino")
	bool bBladesCollide = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mulino")
	TObjectPtr<UMaterialInterface> TowerMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mulino")
	TObjectPtr<UMaterialInterface> BladeMaterial;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Mulino")
	void ApplyWindmillSettings();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mulino")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mulino")
	TObjectPtr<UStaticMeshComponent> Tower;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mulino")
	TObjectPtr<USceneComponent> Head;

	/** Il mozzo: gira sul suo asse X (roll) e si porta dietro le pale. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mulino")
	TObjectPtr<USceneComponent> Hub;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mulino")
	TObjectPtr<UStaticMeshComponent> HubMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mulino")
	TArray<TObjectPtr<UStaticMeshComponent>> Blades;
};
