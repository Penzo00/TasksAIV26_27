// GD13 - Ostacolo rotante: una sbarra che gira e spinge via il personaggio.
// Con bKillOnTouch fa invece ripartire il livello.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/HitResult.h"
#include "SpinningObstacle.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS()
class AIV_UE5INTRO_API ASpinningObstacle : public AActor
{
	GENERATED_BODY()

public:
	ASpinningObstacle();

	/** Gradi al secondo attorno all'asse verticale (negativo = senso opposto). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ostacolo")
	float DegreesPerSecond = 90.f;

	/** Lunghezza, spessore e altezza della sbarra, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ostacolo")
	FVector BarSize = FVector(600.f, 30.f, 30.f);

	/** Altezza della sbarra dal pavimento, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ostacolo")
	float BarHeight = 60.f;

	/** Forza con cui la sbarra spinge via il personaggio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ostacolo", meta = (ClampMin = "0.0"))
	float PushStrength = 900.f;

	/** Se vero, toccare la sbarra fa ripartire il livello invece di spingere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ostacolo")
	bool bKillOnTouch = false;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Ostacolo")
	void ApplyBarSize();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ostacolo")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ostacolo")
	TObjectPtr<UStaticMeshComponent> Bar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ostacolo")
	TObjectPtr<UStaticMeshComponent> Base;

private:
	UFUNCTION()
	void HandleBarOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
