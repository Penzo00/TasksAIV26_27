// GD13 - Classe base (ereditarietà!) di tutte le "zone" del platformer:
// un volume a forma di scatola che reagisce quando il personaggio ci entra o esce.
// Le classi figlie decidono COSA succede: trappola, danno, bonus, traguardo.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/HitResult.h"
#include "PlatformerZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;
class ACharacter;

UCLASS(Abstract)
class AIV_UE5INTRO_API APlatformerZone : public AActor
{
	GENERATED_BODY()

public:
	APlatformerZone();

	/** Metà delle dimensioni della zona, in cm (come la Box Extent di un Box Collision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zona")
	FVector ZoneExtent = FVector(150.f, 150.f, 100.f);

	/** Se vero, reagisce solo al personaggio del giocatore (non ad altri actor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zona")
	bool bOnlyPlayer = true;

	/** Mostra una pedana sul fondo della zona, così la si vede anche in gioco. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zona|Aspetto")
	bool bShowVisual = true;

	/** Spessore della pedana visibile sul fondo della zona, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zona|Aspetto", meta = (ClampMin = "0.5"))
	float VisualThickness = 4.f;

	/** Materiale della pedana visibile (opzionale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zona|Aspetto")
	TObjectPtr<UMaterialInterface> VisualMaterial;

	/** Scritta sopra la zona (vuota = nessuna scritta). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zona|Aspetto")
	FText Label;

	/** Applica ZoneExtent, aspetto e scritta ai componenti (lo fa anche da sola in editor). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Zona")
	void ApplyZoneSettings();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	/** Da ridefinire nelle classi figlie. */
	virtual void OnCharacterEntered(ACharacter* Character) {}
	virtual void OnCharacterExited(ACharacter* Character) {}

	bool IsValidTarget(AActor* Other) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zona")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zona")
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zona")
	TObjectPtr<UTextRenderComponent> LabelText;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
