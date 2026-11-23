#include "PlatformerHealthComponent.h"

#include "PlatformerRunSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"

UPlatformerHealthComponent::UPlatformerHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlatformerHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
}

void UPlatformerHealthComponent::ApplyDamage(float Amount)
{
	if (Amount <= 0.f || IsDead())
	{
		return;
	}
	const float Old = Health;
	Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, Health - Old);

	if (IsDead())
	{
		OnDied.Broadcast();
		if (bRestartLevelOnDeath)
		{
			if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
			{
				Run->KillPlayerAndRestart(RestartDelay);
			}
		}
	}
}

void UPlatformerHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.f || IsDead())
	{
		return;
	}
	const float Old = Health;
	Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health, Health - Old);
}

UPlatformerHealthComponent* UPlatformerHealthComponent::FindOrAdd(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	if (UPlatformerHealthComponent* Existing = Actor->FindComponentByClass<UPlatformerHealthComponent>())
	{
		return Existing;
	}
	UPlatformerHealthComponent* NewComp = NewObject<UPlatformerHealthComponent>(Actor, TEXT("PlatformerHealth"));
	Actor->AddInstanceComponent(NewComp);
	NewComp->RegisterComponent();   // chiama anche BeginPlay, perché il gioco è già partito
	return NewComp;
}
