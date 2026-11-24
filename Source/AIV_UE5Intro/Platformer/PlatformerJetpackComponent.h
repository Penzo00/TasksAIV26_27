// Il jetpack: in aria, TENENDO PREMUTO il tasto di salto, si viene spinti
// verso l'alto finché c'è carburante. Mentre si vola l'Air Control è alto,
// così si sceglie con precisione dove atterrare. Il carburante si ricarica
// a terra. Si sblocca con l'oggetto "JETPACK" (EPlatformerAbility::Jetpack).
//
// Convive con il doppio salto: PREMERE in aria = doppio salto (subito),
// TENERE premuto = dopo HoldDelay secondi parte il jetpack.
//
// In Blueprint: Input Action IA_Jump con gli eventi Started (salva l'ora)
// e Completed (rilascio); nel Tick, se è tenuto premuto da più di HoldDelay,
// il personaggio è "Is Falling" e c'è carburante: Velocity.Z verso
// MaxUpSpeed con FInterp To Constant, carburante -= Delta Seconds.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlatformerJetpackComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UInputAction;
class UEnhancedInputComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMaterialInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlatformerJetpackEvent);

UCLASS(ClassGroup = (Platformer), meta = (BlueprintSpawnableComponent))
class AIV_UE5INTRO_API UPlatformerJetpackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlatformerJetpackComponent();

	/** Se vero, il jetpack funziona anche senza averlo raccolto (per i test). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetpack")
	bool bAlwaysUnlocked = false;

	/** Secondi di volo con il serbatoio pieno. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetpack", meta = (ClampMin = "0.1"))
	float MaxFuel = 2.2f;

	/** Secondi di carburante recuperati per ogni secondo passato a terra. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetpack", meta = (ClampMin = "0.0"))
	float RefillPerSecond = 1.5f;

	/** Dopo quanto tempo di tasto tenuto premuto si accende (s). Così un tocco resta un salto/doppio salto. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetpack", meta = (ClampMin = "0.0"))
	float HoldDelay = 0.2f;

	/** Velocità verso l'alto che il jetpack cerca di raggiungere (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetpack")
	float MaxUpSpeed = 260.f;

	/** Quanto in fretta si arriva a MaxUpSpeed (cm/s²): più alto = più "scattante". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetpack", meta = (ClampMin = "0.0"))
	float Acceleration = 2400.f;

	/** Air Control mentre si vola: alto = si pilota l'atterraggio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetpack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float JetpackAirControl = 0.9f;

	/** L'azione di salto (la stessa di BP_Bot2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetpack|Input")
	TSoftObjectPtr<UInputAction> JumpAction;

	/** Materiale della "fiamma" (opzionale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jetpack|Aspetto")
	TObjectPtr<UMaterialInterface> FlameMaterial;

	UPROPERTY(BlueprintAssignable, Category = "Jetpack")
	FOnPlatformerJetpackEvent OnJetpackStarted;

	UPROPERTY(BlueprintAssignable, Category = "Jetpack")
	FOnPlatformerJetpackEvent OnJetpackStopped;

	UFUNCTION(BlueprintPure, Category = "Jetpack")
	bool IsUnlocked() const;

	UFUNCTION(BlueprintPure, Category = "Jetpack")
	bool IsThrusting() const { return bThrusting; }

	/** Carburante da 0 a 1 (per l'HUD). */
	UFUNCTION(BlueprintPure, Category = "Jetpack")
	float GetFuelPercent() const { return MaxFuel > 0.f ? Fuel / MaxFuel : 0.f; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TryBindInput();
	void HandleJumpPressed();
	void HandleJumpReleased();
	void StartThrust();
	void StopThrust();
	void CreateFlame();

	UPROPERTY()
	TObjectPtr<ACharacter> Character;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> Move;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Flame;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> FlameLight;

	TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
	float Fuel = 0.f;
	float PressedTime = -100.f;
	bool bHeld = false;
	bool bThrusting = false;
	float SavedAirControl = 0.4f;
};
