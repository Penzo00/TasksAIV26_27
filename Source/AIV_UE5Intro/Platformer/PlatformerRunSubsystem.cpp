#include "PlatformerRunSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UPlatformerRunSubsystem* UPlatformerRunSubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext || !GEngine)
	{
		return nullptr;
	}
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UPlatformerRunSubsystem>() : nullptr;
}

void UPlatformerRunSubsystem::KillPlayerAndRestart(float Delay)
{
	if (bRestartPending)
	{
		return;   // una sola morte per volta, anche se si toccano due trappole insieme
	}
	bRestartPending = true;
	++DeathCount;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
			FString::Printf(TEXT("Morto! Morti totali: %d"), DeathCount));
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (World && Delay > 0.f)
	{
		World->GetTimerManager().SetTimer(RestartHandle, this,
			&UPlatformerRunSubsystem::RestartCurrentLevel, Delay, false);
	}
	else
	{
		RestartCurrentLevel();
	}
}

void UPlatformerRunSubsystem::RestartCurrentLevel()
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	bRestartPending = false;
	const FString Current = UGameplayStatics::GetCurrentLevelName(World, true);
	UGameplayStatics::OpenLevel(World, FName(*Current));
}

void UPlatformerRunSubsystem::GoToLevel(FName LevelName)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || LevelName.IsNone())
	{
		return;
	}
	bRestartPending = false;
	UGameplayStatics::OpenLevel(World, LevelName);
}
