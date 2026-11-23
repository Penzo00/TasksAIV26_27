// GD12 - Altri pezzi del platformer visti a lezione:
//   AJumpPad            il trampolino (Launch Character)
//   ACrumblingPlatform  la piattaforma che si distrugge (e poi ricompare)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/HitResult.h"
#include "PlatformerProps.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;

UCLASS()
class AIV_UE5INTRO_API AJumpPad : public AActor
{
	GENERATED_BODY()

public:
	AJumpPad();

	/** Velocità data al personaggio, nello spazio dell'actor (X avanti, Z su). cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trampolino")
	FVector LaunchVelocity = FVector(300.f, 0.f, 1300.f);

	/** Se vero, sostituisce la velocità orizzontale invece di sommarla. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trampolino")
	bool bOverrideHorizontal = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trampolino")
	TObjectPtr<USoundBase> BounceSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trampolino")
	TObjectPtr<UMaterialInterface> PadMaterial;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trampolino")
	TObjectPtr<UStaticMeshComponent> Base;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trampolino")
	TObjectPtr<UStaticMeshComponent> Pad;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trampolino")
	TObjectPtr<UBoxComponent> Trigger;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	float SquashTime = 0.f;
};

UCLASS()
class AIV_UE5INTRO_API ACrumblingPlatform : public AActor
{
	GENERATED_BODY()

public:
	ACrumblingPlatform();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma")
	FVector PlatformSize = FVector(250.f, 250.f, 30.f);

	/** Secondi in cui trema prima di cadere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma", meta = (ClampMin = "0.0"))
	float ShakeTime = 0.8f;

	/** Quanto trema (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma", meta = (ClampMin = "0.0"))
	float ShakeAmount = 4.f;

	/** Secondi prima di ricomparire (0 = non ricompare). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma", meta = (ClampMin = "0.0"))
	float RespawnTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma")
	TObjectPtr<UMaterialInterface> PlatformMaterial;

	/** Materiale mentre trema (avviso visivo, opzionale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma")
	TObjectPtr<UMaterialInterface> WarningMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Piattaforma")
	TObjectPtr<USoundBase> CrumbleSound;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Piattaforma")
	void ApplyPlatformSize();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Piattaforma")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Piattaforma")
	TObjectPtr<UBoxComponent> Trigger;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void Fall();
	void Respawn();

	enum class EState : uint8 { Idle, Shaking, Gone };
	EState State = EState::Idle;
	float StateTime = 0.f;
	FTimerHandle RespawnHandle;
};
