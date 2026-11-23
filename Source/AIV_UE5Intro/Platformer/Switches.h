// UE7/UE12 - Gli interruttori. Una classe padre (ASwitchBase) con:
//   * un ARRAY di attivabili da chiamare (primo metodo della lezione);
//   * un EVENT DISPATCHER (OnSwitchChanged) a cui gli attivabili si legano
//     con Bind Event (secondo metodo).
// Due figli:
//   APressurePlate  il pulsante a pavimento (UE7): si abbassa con una
//                   "timeline" quando ci si sale sopra
//   AInteractLever  la leva che si tira con il tasto E (UE12, interfaccia)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/HitResult.h"
#include "PlatformerInteractable.h"
#include "Switches.generated.h"

class AActivable;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSwitchChanged, bool, bOn);

UCLASS(Abstract)
class AIV_UE5INTRO_API ASwitchBase : public AActor
{
	GENERATED_BODY()

public:
	ASwitchBase();

	/** Gli attivabili da chiamare (primo metodo: For Each Loop sull'array). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interruttore")
	TArray<TObjectPtr<AActivable>> Targets;

	/** L'event dispatcher (secondo metodo): ci si lega con Bind Event. */
	UPROPERTY(BlueprintAssignable, Category = "Interruttore")
	FOnSwitchChanged OnSwitchChanged;

	/** Suono quando cambia stato (opzionale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interruttore")
	TObjectPtr<USoundBase> ClickSound;

	/** Materiale "acceso" e "spento" della parte che si muove (opzionali). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interruttore")
	TObjectPtr<UMaterialInterface> OnMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interruttore")
	TObjectPtr<UMaterialInterface> OffMaterial;

	UFUNCTION(BlueprintCallable, Category = "Interruttore")
	void SetSwitchOn(bool bOn);

	UFUNCTION(BlueprintPure, Category = "Interruttore")
	bool IsOn() const { return bIsOn; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interruttore")
	TObjectPtr<USceneComponent> Root;

	/** La parte che si muove o cambia colore (pedana o leva). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interruttore")
	TObjectPtr<UStaticMeshComponent> Handle;

	virtual void OnSwitchStateChanged() {}

	bool bIsOn = false;
};

/** Il pulsante a pavimento: premuto finché c'è qualcuno sopra (o per sempre). */
UCLASS()
class AIV_UE5INTRO_API APressurePlate : public ASwitchBase
{
	GENERATED_BODY()

public:
	APressurePlate();

	/** Se vero, resta premuto anche dopo che si scende. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulsante")
	bool bStayPressed = true;

	/** Quanto si abbassa la pedana (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulsante")
	float PressDepth = 8.f;

	/** Secondi della "timeline" di discesa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pulsante", meta = (ClampMin = "0.01"))
	float PressTime = 0.25f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pulsante")
	TObjectPtr<UStaticMeshComponent> Frame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pulsante")
	TObjectPtr<UBoxComponent> Trigger;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	float PressAlpha = 0.f;
	float HandleRestZ = 0.f;
};

/** La leva: si tira con il tasto E (implementa l'interfaccia di interazione). */
UCLASS()
class AIV_UE5INTRO_API AInteractLever : public ASwitchBase, public IPlatformerInteractable
{
	GENERATED_BODY()

public:
	AInteractLever();

	/** Testo mostrato dall'HUD vicino alla leva. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leva")
	FText Prompt = FText::FromString(TEXT("Tira la leva"));

	/** Se vero, ogni pressione cambia stato; se falso si tira una volta sola. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leva")
	bool bToggle = true;

	/** Scritta sopra la leva (vuota = nessuna). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Leva")
	FText Label;

	virtual void Interact_Implementation(ACharacter* ByCharacter) override;
	virtual FText GetInteractPrompt_Implementation() const override;

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void OnSwitchStateChanged() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Leva")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Leva")
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Leva")
	TObjectPtr<UTextRenderComponent> LabelText;
};
