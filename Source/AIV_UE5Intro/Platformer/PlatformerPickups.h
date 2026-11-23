// GD7/UE8 - I collezionabili. Una classe padre che gira su se stessa e
// ondeggia (Event Tick + Delta Seconds, "le monete che ruotano") e si
// raccoglie passandoci attraverso (overlap + cast al personaggio).
// I figli decidono che cosa succede quando la si raccoglie:
//   ACoinPickup     una moneta (il conteggio sta nella Game Instance)
//   AAbilityPickup  sblocca un'abilità (doppio salto, wall run)
//   AHealthPickup   ridà vita

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/HitResult.h"
#include "PlatformerRunSubsystem.h"
#include "PlatformerPickups.generated.h"

class ACharacter;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class UMaterialInterface;
class USoundBase;

UCLASS(Abstract)
class AIV_UE5INTRO_API APlatformerPickup : public AActor
{
	GENERATED_BODY()

public:
	APlatformerPickup();

	/** Gradi al secondo di rotazione. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collezionabile")
	float SpinSpeed = 120.f;

	/** Ampiezza dell'ondeggiamento verticale (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collezionabile")
	float BobHeight = 12.f;

	/** Velocità dell'ondeggiamento (cicli al secondo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collezionabile")
	float BobSpeed = 0.8f;

	/** Raggio di raccolta (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collezionabile", meta = (ClampMin = "5.0"))
	float PickupRadius = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collezionabile")
	TObjectPtr<UMaterialInterface> Material;

	/** Luce piccola che fa "brillare" l'oggetto e lo fa notare (0 = spenta). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collezionabile", meta = (ClampMin = "0.0"))
	float GlowIntensity = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collezionabile")
	FLinearColor GlowColor = FLinearColor(1.f, 0.8f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collezionabile")
	TObjectPtr<USoundBase> PickupSound;

	/** Scritta sopra l'oggetto (vuota = nessuna). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collezionabile")
	FText Label;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Collezionabile")
	void ApplyPickupSettings();

	/** Evento per i Blueprint figli. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Collezionabile")
	void OnPickedUp(ACharacter* ByCharacter);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Da ridefinire: cosa succede. Restituire falso per NON consumare l'oggetto. */
	virtual bool Collect(ACharacter* Character) { return true; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collezionabile")
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collezionabile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collezionabile")
	TObjectPtr<UPointLightComponent> Glow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collezionabile")
	TObjectPtr<UTextRenderComponent> LabelText;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	float Clock = 0.f;
	float MeshBaseZ = 0.f;
	bool bCollected = false;
};

/** La moneta: si somma al conto del livello; diventa tua al traguardo. */
UCLASS()
class AIV_UE5INTRO_API ACoinPickup : public APlatformerPickup
{
	GENERATED_BODY()

public:
	ACoinPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moneta", meta = (ClampMin = "1"))
	int32 Value = 1;

protected:
	virtual bool Collect(ACharacter* Character) override;
};

/** Sblocca un'abilità: il doppio salto nel livello 1. */
UCLASS()
class AIV_UE5INTRO_API AAbilityPickup : public APlatformerPickup
{
	GENERATED_BODY()

public:
	AAbilityPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita")
	EPlatformerAbility Ability = EPlatformerAbility::DoubleJump;

	/** Messaggio a schermo quando la si prende. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita")
	FString Message = TEXT("Doppio salto sbloccato! Premi salto di nuovo in aria");

	/** Se vero, sparisce quando l'abilità è già stata sbloccata (per esempio rigiocando il livello). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilita")
	bool bHideIfAlreadyUnlocked = true;

protected:
	virtual void BeginPlay() override;
	virtual bool Collect(ACharacter* Character) override;
};

/** Ridà vita al personaggio. */
UCLASS()
class AIV_UE5INTRO_API AHealthPickup : public APlatformerPickup
{
	GENERATED_BODY()

public:
	AHealthPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vita", meta = (ClampMin = "0.0"))
	float HealAmount = 40.f;

protected:
	virtual bool Collect(ACharacter* Character) override;
};
