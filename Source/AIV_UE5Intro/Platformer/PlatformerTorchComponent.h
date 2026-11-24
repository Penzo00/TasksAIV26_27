// La torcia in prima persona.
// Con la rotellina del mouse si accorcia o si allunga il braccio della
// telecamera (Spring Arm / "lever arm") di Bot2. Quando è quasi a zero la
// visuale diventa in prima persona: il corpo sparisce (Owner No See), la
// telecamera sale all'altezza degli occhi, il personaggio guarda dove
// guarda la telecamera e in mano compare la torcia (un cilindro con una
// Spot Light). La torcia ha una batteria: si scarica SOLO in prima persona,
// sfarfalla quando è quasi vuota e a zero si spegne. Le batterie
// (ABatteryPickup) la ricaricano; fuori dalla prima persona si ricarica
// lentamente (RechargePerSecond, 0 = mai).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "PlatformerTorchComponent.generated.h"

class ACharacter;
class UCameraComponent;
class USpringArmComponent;
class USpotLightComponent;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;
class UMaterialInterface;
class UPrimitiveComponent;

UCLASS(ClassGroup = (Platformer), meta = (BlueprintSpawnableComponent))
class AIV_UE5INTRO_API UPlatformerTorchComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlatformerTorchComponent();

	// ------------------------------------------------------------ telecamera

	/** Quanti cm cambia il braccio della telecamera per ogni "scatto" della rotellina. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Telecamera", meta = (ClampMin = "1.0"))
	float ZoomStep = 60.f;

	/** Braccio massimo (cm). 0 = quello che ha Bot2 all'inizio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Telecamera", meta = (ClampMin = "0.0"))
	float MaxArmLength = 0.f;

	/** Sotto questa lunghezza del braccio si è in prima persona (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Telecamera", meta = (ClampMin = "0.0"))
	float FirstPersonThreshold = 40.f;

	/** Velocità con cui il braccio raggiunge la lunghezza voluta (FInterp To). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Telecamera", meta = (ClampMin = "0.1"))
	float ZoomInterpSpeed = 10.f;

	/** In prima persona la telecamera sale di tanto, all'altezza degli occhi (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Telecamera")
	float EyeHeight = 65.f;

	/** Azione per lo zoom (asse 1D). Vuota = la crea il componente, legata alla rotellina. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Telecamera")
	TObjectPtr<UInputAction> ZoomAction;

	// ------------------------------------------------------------ batteria

	/** Secondi di luce con la batteria piena. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Batteria", meta = (ClampMin = "1.0"))
	float MaxBattery = 25.f;

	/** Secondi di batteria recuperati per ogni secondo FUORI dalla prima persona (0 = solo con le batterie). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Batteria", meta = (ClampMin = "0.0"))
	float RechargePerSecond = 0.25f;

	/** Sotto questa percentuale la luce sfarfalla (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Batteria", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowBatteryThreshold = 0.2f;

	// ------------------------------------------------------------ luce

	/** Intensità della torcia (candele). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Luce", meta = (ClampMin = "0.0"))
	float LightIntensity = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Luce", meta = (ClampMin = "100.0"))
	float LightRange = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Luce", meta = (ClampMin = "1.0", ClampMax = "80.0"))
	float ConeAngle = 26.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Luce")
	FLinearColor LightColor = FLinearColor(1.f, 0.93f, 0.8f);

	/** Materiale del corpo della torcia (opzionale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torcia|Luce")
	TObjectPtr<UMaterialInterface> TorchMaterial;

	// ------------------------------------------------------------ stato

	UFUNCTION(BlueprintPure, Category = "Torcia")
	bool IsFirstPerson() const { return bFirstPerson; }

	UFUNCTION(BlueprintPure, Category = "Torcia")
	bool IsLightOn() const { return bFirstPerson && Battery > 0.f; }

	/** Batteria da 0 a 1 (per l'HUD). */
	UFUNCTION(BlueprintPure, Category = "Torcia")
	float GetBatteryPercent() const { return MaxBattery > 0.f ? Battery / MaxBattery : 0.f; }

	/** Aggiunge secondi di batteria (lo chiamano le batterie raccolte). */
	UFUNCTION(BlueprintCallable, Category = "Torcia")
	void AddBattery(float Seconds);

	/** Cambia la lunghezza voluta del braccio (positivo = avvicina, come la rotellina in avanti). */
	UFUNCTION(BlueprintCallable, Category = "Torcia")
	void Zoom(float Steps);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TryBindInput();
	void HandleZoom(const FInputActionValue& Value);
	void CreateTorch();
	void SetFirstPerson(bool bEnable);

	UPROPERTY()
	TObjectPtr<ACharacter> Character;

	UPROPERTY()
	TObjectPtr<USpringArmComponent> Arm;

	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TorchMesh;

	UPROPERTY()
	TObjectPtr<USpotLightComponent> TorchLight;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> ZoomContext;

	TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> HiddenComponents;
	bool bContextAdded = false;
	bool bFirstPerson = false;
	bool bSavedUseControllerYaw = false;
	bool bSavedOrientToMovement = true;
	float DesiredArmLength = 400.f;
	float StartArmLength = 400.f;
	FVector StartTargetOffset = FVector::ZeroVector;
	float Battery = 0.f;
};
