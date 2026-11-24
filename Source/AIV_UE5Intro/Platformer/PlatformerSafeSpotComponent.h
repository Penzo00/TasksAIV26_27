// Il "posto sicuro" e le vite.
// Mentre il personaggio cammina, il componente si ricorda l'ultimo punto
// in cui stava BENE a terra: pavimento fermo (non una piattaforma che si
// muove o crolla), non sul bordo (quattro trace verso il basso intorno ai
// piedi devono trovare tutte il pavimento). Se il personaggio cade in una
// AFallZone (o semplicemente precipita molto più in basso dell'ultimo posto
// sicuro) perde una vita e torna lì. Con zero vite: game over, si riparte
// dal primo livello (lo decide UPlatformerRunSubsystem).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlatformerSafeSpotComponent.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS(ClassGroup = (Platformer), meta = (BlueprintSpawnableComponent))
class AIV_UE5INTRO_API UPlatformerSafeSpotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlatformerSafeSpotComponent();

	/** Ogni quanti secondi si controlla se il punto attuale è "sicuro". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Posto sicuro", meta = (ClampMin = "0.02"))
	float SampleInterval = 0.2f;

	/** Distanza dai piedi dei quattro trace di controllo del bordo (cm). Più grande = più lontano dai bordi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Posto sicuro", meta = (ClampMin = "0.0"))
	float EdgeMargin = 50.f;

	/** Gli actor con questo tag non sono mai un posto sicuro (casse generate, trappole...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Posto sicuro")
	FName NoSafeSpotTag = FName(TEXT("NoSafeSpot"));

	/** Se si scende più di così sotto l'ultimo posto sicuro (cm), conta come caduta anche senza zona. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Posto sicuro", meta = (ClampMin = "100.0"))
	float MaxFallDistance = 2500.f;

	/** Vero se ha già trovato almeno un posto sicuro (altrimenti si torna al punto di partenza). */
	UFUNCTION(BlueprintPure, Category = "Posto sicuro")
	FVector GetLastSafeLocation() const { return LastSafeLocation; }

	/** Caduta: toglie una vita e riporta all'ultimo posto sicuro (o game over). */
	UFUNCTION(BlueprintCallable, Category = "Posto sicuro")
	void HandleFall();

	/** Riporta all'ultimo posto sicuro senza togliere vite. */
	UFUNCTION(BlueprintCallable, Category = "Posto sicuro")
	void TeleportToSafeSpot();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	bool IsCurrentSpotSafe() const;

	UPROPERTY()
	TObjectPtr<ACharacter> Character;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> Move;

	FVector LastSafeLocation = FVector::ZeroVector;
	FRotator LastSafeRotation = FRotator::ZeroRotator;
	float NextSample = 0.f;
	float IgnoreFallsUntil = 0.f;
};
