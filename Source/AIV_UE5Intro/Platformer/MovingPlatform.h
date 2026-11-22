// GD13 - Piattaforma mobile: va avanti e indietro tra la posizione iniziale
// e un punto di arrivo, con una pausa agli estremi. È la piattaforma della
// lezione (Event Tick + Lerp), scritta in C++.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovingPlatform.generated.h"

class UStaticMeshComponent;

UCLASS()
class AIV_UE5INTRO_API AMovingPlatform : public AActor
{
	GENERATED_BODY()

public:
	AMovingPlatform();

	/** Spostamento del punto di arrivo rispetto alla posizione iniziale (si trascina col widget nel viewport). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma", meta = (MakeEditWidget = "true"))
	FVector EndOffset = FVector(0.f, 400.f, 0.f);

	/** Secondi per andare da un estremo all'altro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma", meta = (ClampMin = "0.1"))
	float TravelTime = 2.f;

	/** Secondi di sosta a ogni estremo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma", meta = (ClampMin = "0.0"))
	float PauseTime = 0.5f;

	/** Ritardo iniziale: sfasa piattaforme uguali messe in fila. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma", meta = (ClampMin = "0.0"))
	float StartDelay = 0.f;

	/** Movimento morbido agli estremi (ease in/out) invece che lineare. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma")
	bool bSmooth = true;

	/** Dimensioni della piattaforma in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma")
	FVector PlatformSize = FVector(300.f, 300.f, 30.f);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Piattaforma")
	void ApplyPlatformSize();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Piattaforma")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	FVector StartLocation = FVector::ZeroVector;
	float Clock = 0.f;
};
