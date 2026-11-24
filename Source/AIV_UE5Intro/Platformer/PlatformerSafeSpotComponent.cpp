#include "PlatformerSafeSpotComponent.h"

#include "Activable.h"
#include "MovingPlatform.h"
#include "PlatformerProps.h"
#include "PlatformerRunSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

UPlatformerSafeSpotComponent::UPlatformerSafeSpotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlatformerSafeSpotComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	Move = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Character || !Move)
	{
		SetComponentTickEnabled(false);
		return;
	}
	// finché non si trova un posto migliore, il posto sicuro è la partenza
	LastSafeLocation = Character->GetActorLocation();
	LastSafeRotation = Character->GetActorRotation();
}

bool UPlatformerSafeSpotComponent::IsCurrentSpotSafe() const
{
	if (!Move->IsMovingOnGround() || !Move->CurrentFloor.IsWalkableFloor())
	{
		return false;
	}
	const AActor* Floor = Move->CurrentFloor.HitResult.GetActor();
	if (Floor)
	{
		// niente piattaforme che si muovono, crollano o spariscono
		if (Floor->ActorHasTag(NoSafeSpotTag) || Floor->IsA<AMovingPlatform>() || Floor->IsA<ACrumblingPlatform>()
			|| Floor->IsA<AActivable>() || Floor->GetVelocity().SizeSquared() > 1.f)
		{
			return false;
		}
	}

	// non sul bordo: il pavimento deve esserci tutto intorno ai piedi
	const UWorld* World = GetWorld();
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Feet = Character->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlatformerSafeSpot), false, Character);
	const FVector Offsets[] = { FVector(EdgeMargin, 0, 0), FVector(-EdgeMargin, 0, 0), FVector(0, EdgeMargin, 0), FVector(0, -EdgeMargin, 0) };
	for (const FVector& Offset : Offsets)
	{
		FHitResult Hit;
		const FVector Start = Feet + Offset;
		const FVector End = Start - FVector(0.f, 0.f, HalfHeight + 40.f);
		if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) || Hit.ImpactNormal.Z < 0.7f)
		{
			return false;
		}
		if (const AActor* Other = Hit.GetActor())
		{
			if (Other->ActorHasTag(NoSafeSpotTag))
			{
				return false;
			}
		}
	}
	return true;
}

void UPlatformerSafeSpotComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Character || !Move)
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextSample)
	{
		NextSample = Now + SampleInterval;
		if (IsCurrentSpotSafe())
		{
			LastSafeLocation = Character->GetActorLocation();
			LastSafeRotation = Character->GetActorRotation();
		}
	}
	// caduta "nel vuoto" senza una zona sotto
	if (Character->GetActorLocation().Z < LastSafeLocation.Z - MaxFallDistance)
	{
		HandleFall();
	}
}

void UPlatformerSafeSpotComponent::TeleportToSafeSpot()
{
	if (!Character || !Move)
	{
		return;
	}
	Move->StopMovementImmediately();
	Character->SetActorLocationAndRotation(LastSafeLocation + FVector(0.f, 0.f, 5.f), LastSafeRotation, false, nullptr,
		ETeleportType::TeleportPhysics);
	if (AController* Controller = Character->GetController())
	{
		Controller->SetControlRotation(LastSafeRotation);
	}
}

void UPlatformerSafeSpotComponent::HandleFall()
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now < IgnoreFallsUntil)
	{
		return;   // si è appena tornati al posto sicuro: una caduta alla volta
	}
	IgnoreFallsUntil = Now + 0.5f;

	UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	if (!Run)
	{
		TeleportToSafeSpot();
		return;
	}
	if (Run->LoseLife())
	{
		TeleportToSafeSpot();
	}
	else if (Move)
	{
		// game over: il personaggio si ferma mentre parte il cambio di livello
		Move->DisableMovement();
		IgnoreFallsUntil = Now + 100.f;
	}
}
