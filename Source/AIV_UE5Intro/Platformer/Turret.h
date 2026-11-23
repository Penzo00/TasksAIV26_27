// UE4/UE5 - La torretta e il proiettile.
//  * una Sphere Collision fa da raggio d'azione (overlap = bersaglio trovato,
//    end overlap = la torretta si ferma);
//  * la testa ruota verso il player con Find Look at Rotation e RInterp To,
//    con un Clamp sul pitch;
//  * prima di sparare un Line Trace controlla che il player sia "in vista";
//  * spara con un timer (Set Timer by Event) e mira ai piedi;
//  * il proiettile usa il Projectile Movement e fa danno con Apply Damage.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/HitResult.h"
#include "Turret.generated.h"

class ACharacter;
class UProjectileMovementComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;

UCLASS()
class AIV_UE5INTRO_API APlatformerProjectile : public AActor
{
	GENERATED_BODY()

public:
	APlatformerProjectile();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Proiettile")
	float Damage = 25.f;

	/** Spinta data al personaggio colpito (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Proiettile")
	float Knockback = 500.f;

	UFUNCTION(BlueprintCallable, Category = "Proiettile")
	void SetProjectileMaterial(UMaterialInterface* Material);

	UProjectileMovementComponent* GetMovement() const { return Movement; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proiettile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proiettile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proiettile")
	TObjectPtr<UProjectileMovementComponent> Movement;

private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);
};

UCLASS()
class AIV_UE5INTRO_API ATurret : public AActor
{
	GENERATED_BODY()

public:
	ATurret();

	/** Raggio d'azione (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta", meta = (ClampMin = "100.0"))
	float Range = 1800.f;

	/** Secondi fra un colpo e l'altro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta", meta = (ClampMin = "0.1"))
	float FireInterval = 1.4f;

	/** Velocità di rotazione della testa (RInterp To). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta", meta = (ClampMin = "0.1"))
	float TurnSpeed = 3.f;

	/** Clamp del pitch della canna (gradi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta")
	float MinPitch = -40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta")
	float MaxPitch = 30.f;

	/** Spara solo se la canna è entro questo angolo dal bersaglio (gradi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta", meta = (ClampMin = "1.0"))
	float FireConeDegrees = 12.f;

	/** Dove mirare rispetto al centro del personaggio: negativo = ai piedi (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta")
	float AimHeightOffset = -60.f;

	/** Se vero, mira dove il personaggio SARÀ (anticipa il movimento). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta")
	bool bLeadTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta|Proiettile", meta = (ClampMin = "100.0"))
	float ProjectileSpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta|Proiettile", meta = (ClampMin = "0.0"))
	float ProjectileDamage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta|Proiettile")
	TSubclassOf<APlatformerProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta|Aspetto")
	TObjectPtr<UMaterialInterface> BodyMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta|Aspetto")
	TObjectPtr<UMaterialInterface> ProjectileMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torretta|Aspetto")
	TObjectPtr<USoundBase> FireSound;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Torretta")
	void ApplyTurretSettings();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torretta")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torretta")
	TObjectPtr<UStaticMeshComponent> Base;

	/** La testa: ruota in yaw. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torretta")
	TObjectPtr<USceneComponent> Head;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torretta")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	/** La canna: ruota in pitch. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torretta")
	TObjectPtr<UStaticMeshComponent> Barrel;

	/** La bocca da cui escono i proiettili. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torretta")
	TObjectPtr<USceneComponent> Muzzle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Torretta")
	TObjectPtr<USphereComponent> Detection;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void Shoot();
	FVector GetAimPoint() const;
	bool HasLineOfSight() const;

	TWeakObjectPtr<ACharacter> Target;
	FTimerHandle FireHandle;
	float CurrentPitch = 0.f;
};
