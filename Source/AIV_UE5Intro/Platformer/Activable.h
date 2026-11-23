// UE7 - Gli "attivabili": una classe padre (AActivable) con Activate e
// Deactivate, e tre figli che ridefiniscono cosa succede (ereditarietà):
//   AActivableDoor     la doppia porta (GD12) che si apre con una "timeline"
//   AActivableLight    la luce che si accende (salva la sua intensità)
//   AActivableMover    la piattaforma/ascensore che va al punto di arrivo
// Si attivano in due modi, come a lezione:
//   1) l'interruttore ha un ARRAY di attivabili e li chiama uno per uno;
//   2) l'attivabile si mette IN ASCOLTO dell'event dispatcher di un
//      interruttore (ListenTo) con Bind Event.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Activable.generated.h"

class ASwitchBase;
class UCurveFloat;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;

UCLASS(Abstract)
class AIV_UE5INTRO_API AActivable : public AActor
{
	GENERATED_BODY()

public:
	AActivable();

	/** Stato all'avvio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attivabile")
	bool bStartActive = false;

	/** Interruttori di cui ascoltare l'event dispatcher (secondo metodo della lezione). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attivabile")
	TArray<TObjectPtr<ASwitchBase>> ListenTo;

	/** Secondi della "timeline" di attivazione (porta che si apre, piattaforma che sale...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attivabile", meta = (ClampMin = "0.01"))
	float TransitionTime = 1.f;

	/** Curva della timeline (opzionale): senza curva il movimento è morbido (ease in/out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attivabile")
	TObjectPtr<UCurveFloat> TransitionCurve;

	/** Suono all'attivazione (opzionale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attivabile")
	TObjectPtr<USoundBase> ActivateSound;

	UFUNCTION(BlueprintCallable, Category = "Attivabile")
	void SetActive(bool bNewActive);

	UFUNCTION(BlueprintCallable, Category = "Attivabile")
	void Toggle() { SetActive(!bActive); }

	UFUNCTION(BlueprintPure, Category = "Attivabile")
	bool IsActive() const { return bActive; }

	/** Evento per i Blueprint figli. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Attivabile")
	void OnActivationChanged(bool bNowActive);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Da ridefinire nei figli: Alpha va da 0 (spento) a 1 (attivo). */
	virtual void ApplyAlpha(float Alpha) {}

	UFUNCTION()
	void HandleSwitchChanged(bool bOn);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attivabile")
	TObjectPtr<USceneComponent> Root;

	bool bActive = false;
	float Progress = 0.f;   // 0..1, avanza verso bActive con il tempo
};

/** La doppia porta: due ante che scorrono di lato o ruotano sui cardini. */
UCLASS()
class AIV_UE5INTRO_API AActivableDoor : public AActivable
{
	GENERATED_BODY()

public:
	AActivableDoor();

	/** Larghezza totale del vano (le due ante insieme), in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porta", meta = (ClampMin = "10.0"))
	float DoorWidth = 240.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porta", meta = (ClampMin = "10.0"))
	float DoorHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porta", meta = (ClampMin = "1.0"))
	float DoorThickness = 20.f;

	/** Vero: le ante scorrono di lato. Falso: ruotano sui cardini. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porta")
	bool bSliding = true;

	/** Gradi di apertura se ruotano. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porta")
	float OpenAngle = 95.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Porta")
	TObjectPtr<UMaterialInterface> DoorMaterial;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Porta")
	void ApplyDoorSize();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void ApplyAlpha(float Alpha) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Porta")
	TObjectPtr<USceneComponent> LeftHinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Porta")
	TObjectPtr<USceneComponent> RightHinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Porta")
	TObjectPtr<UStaticMeshComponent> LeftPanel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Porta")
	TObjectPtr<UStaticMeshComponent> RightPanel;
};

/** La luce attivabile: all'avvio salva la sua intensità, poi la accende e spegne. */
UCLASS()
class AIV_UE5INTRO_API AActivableLight : public AActivable
{
	GENERATED_BODY()

public:
	AActivableLight();

	/** Il bulbo visibile: la luce "motivata" (opzionale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Luce")
	bool bShowBulb = true;

protected:
	virtual void BeginPlay() override;
	virtual void ApplyAlpha(float Alpha) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Luce")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Luce")
	TObjectPtr<UStaticMeshComponent> Bulb;

private:
	float SavedIntensity = 0.f;
};

/** Piattaforma / ascensore: va da dove si trova al punto di arrivo quando è attiva. */
UCLASS()
class AIV_UE5INTRO_API AActivableMover : public AActivable
{
	GENERATED_BODY()

public:
	AActivableMover();

	/** Punto di arrivo, relativo (si trascina con il rombo nel viewport). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma", meta = (MakeEditWidget = "true"))
	FVector EndOffset = FVector(0.f, 0.f, 400.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma")
	FVector PlatformSize = FVector(300.f, 300.f, 30.f);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Piattaforma")
	void ApplyPlatformSize();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void ApplyAlpha(float Alpha) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Piattaforma")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
