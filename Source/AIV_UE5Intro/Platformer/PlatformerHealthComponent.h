// GD13 - Componente "vita": si aggiunge a qualunque actor che può subire danni.
// Se l'actor non lo ha (per esempio BP_Bot2), le zone di danno lo aggiungono da sole.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlatformerHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlatformerHealthChanged, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlatformerDied);

UCLASS(ClassGroup = (Platformer), meta = (BlueprintSpawnableComponent))
class AIV_UE5INTRO_API UPlatformerHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlatformerHealthComponent();

	/** Vita massima (e iniziale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vita", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	/** Se vero, quando la vita arriva a zero il livello riparte. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vita")
	bool bRestartLevelOnDeath = true;

	/** Secondi di attesa prima di far ripartire il livello. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vita", meta = (ClampMin = "0.0"))
	float RestartDelay = 0.5f;

	UPROPERTY(BlueprintAssignable, Category = "Vita")
	FOnPlatformerHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Vita")
	FOnPlatformerDied OnDied;

	UFUNCTION(BlueprintCallable, Category = "Vita")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Vita")
	void Heal(float Amount);

	UFUNCTION(BlueprintPure, Category = "Vita")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Vita")
	bool IsDead() const { return Health <= 0.f; }

	/** Restituisce il componente dell'actor, creandolo se manca. */
	static UPlatformerHealthComponent* FindOrAdd(AActor* Actor);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Vita")
	float Health = 100.f;
};
