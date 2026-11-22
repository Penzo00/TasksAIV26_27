#include "Platformer/LevelTimer.h"

#include "Platformer/PlatformerRunSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"

ALevelTimer::ALevelTimer()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	WorldText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("WorldText"));
	WorldText->SetupAttachment(RootComponent);
	WorldText->SetHorizontalAlignment(EHTA_Center);
	WorldText->SetWorldSize(80.f);
	WorldText->SetTextRenderColor(FColor::Yellow);
	WorldText->SetText(FText::FromString(TEXT("TIMER")));
}

void ALevelTimer::BeginPlay()
{
	Super::BeginPlay();
	TimeLeft = TimeLimit;
	bRunning = true;
	WorldText->SetVisibility(bShowInWorld);
}

void ALevelTimer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bRunning)
	{
		return;
	}

	// Il Delta Time della lezione: si sottrae il tempo reale passato dall'ultimo
	// frame, così il timer va alla stessa velocità a 30 o a 144 fps.
	TimeLeft = FMath::Max(0.f, TimeLeft - DeltaSeconds);

	const FString Text = FString::Printf(TEXT("Tempo: %.1f"), TimeLeft);
	const bool bWarning = TimeLeft <= WarningThreshold;

	if (bShowInWorld)
	{
		WorldText->SetText(FText::FromString(Text));
		WorldText->SetTextRenderColor(bWarning ? FColor::Red : FColor::Yellow);
	}
	if (bShowOnScreen && GEngine)
	{
		// stessa chiave ogni frame: il messaggio si aggiorna invece di accumularsi
		GEngine->AddOnScreenDebugMessage(1402, 0.f, bWarning ? FColor::Red : FColor::Yellow, Text);
	}

	if (TimeLeft <= 0.f)
	{
		bRunning = false;
		if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
		{
			Run->KillPlayerAndRestart(0.5f);
		}
	}
}
