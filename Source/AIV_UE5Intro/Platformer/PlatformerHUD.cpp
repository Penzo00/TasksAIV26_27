#include "PlatformerHUD.h"

#include "LevelTimer.h"
#include "PlatformerAbilitiesComponent.h"
#include "PlatformerCrateSpawnerComponent.h"
#include "PlatformerJetpackComponent.h"
#include "PlatformerTorchComponent.h"
#include "PlatformerHealthComponent.h"
#include "PlatformerInteractable.h"
#include "PlatformerRunSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

void APlatformerHUD::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<ALevelTimer> It(GetWorld()); It; ++It)
	{
		Timer = *It;
		break;
	}
	if (UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		Run->OnMessage.AddDynamic(this, &APlatformerHUD::ShowMessage);
	}
}

void APlatformerHUD::ShowMessage(const FString& Message, FLinearColor Color)
{
	FHudMessage& New = Messages.AddDefaulted_GetRef();
	New.Text = Message;
	New.Color = Color;
	New.ExpireTime = GetWorld() ? GetWorld()->GetTimeSeconds() + MessageDuration : MessageDuration;
	if (Messages.Num() > 4)
	{
		Messages.RemoveAt(0);
	}
}

void APlatformerHUD::DrawLine(const FString& Text, const FLinearColor& Color, float X, float& Y, bool bRightAligned)
{
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, Font, TextScale);
	const float DrawX = bRightAligned ? X - W : X;
	// ombra scura sotto il testo: si legge su qualunque sfondo
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.8f), DrawX + 2.f, Y + 2.f, Font, TextScale);
	DrawText(Text, Color, DrawX, Y, Font, TextScale);
	Y += H + 4.f;
}

void APlatformerHUD::DrawBar(float Percent, const FLinearColor& Color, float X, float& Y)
{
	const float Pct = FMath::Clamp(Percent, 0.f, 1.f);
	DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.8f), X, Y, 288.f, 10.f);
	DrawRect(Color, X, Y, 288.f * Pct, 10.f);
	Y += 16.f;
}

void APlatformerHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;
	const UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	const APawn* Pawn = GetOwningPawn();
	const UPlatformerAbilitiesComponent* Abilities = Pawn ? Pawn->FindComponentByClass<UPlatformerAbilitiesComponent>() : nullptr;
	const UPlatformerHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UPlatformerHealthComponent>() : nullptr;
	const UPlatformerJetpackComponent* Jetpack = Pawn ? Pawn->FindComponentByClass<UPlatformerJetpackComponent>() : nullptr;
	const UPlatformerTorchComponent* Torch = Pawn ? Pawn->FindComponentByClass<UPlatformerTorchComponent>() : nullptr;
	const UPlatformerCrateSpawnerComponent* Crates = Pawn ? Pawn->FindComponentByClass<UPlatformerCrateSpawnerComponent>() : nullptr;

	// ------------------------------------------------ pannello in alto a destra
	if (bShowPanel && Run)
	{
		const float Right = SW - 30.f;
		float Y = 24.f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f), SW - 330.f, 14.f, 316.f, PanelHeight);

		const int32 Pending = Run->GetPendingCoins();
		DrawLine(FString::Printf(TEXT("Monete: %d%s"), Run->GetVisibleCoins(),
			Pending > 0 ? *FString::Printf(TEXT("  (+%d da portare al traguardo)"), Pending) : TEXT("")),
			FLinearColor(1.f, 0.85f, 0.25f), Right, Y, true);
		DrawLine(FString::Printf(TEXT("Morti: %d"), Run->GetDeathCount()), FLinearColor(1.f, 0.5f, 0.45f), Right, Y, true);
		{
			FString Hearts;
			for (int32 i = 0; i < Run->GetMaxLives(); ++i)
			{
				Hearts += i < Run->GetLives() ? TEXT("O ") : TEXT("- ");
			}
			DrawLine(FString::Printf(TEXT("Vite: %s(%d/%d)"), *Hearts, Run->GetLives(), Run->GetMaxLives()),
				FLinearColor(1.f, 0.4f, 0.55f), Right, Y, true);
		}

		const float Best = Run->GetBestTimeForCurrentLevel();
		DrawLine(FString::Printf(TEXT("Tempo: %.1f s%s"), Run->GetLevelElapsedTime(),
			Best > 0.f ? *FString::Printf(TEXT("   record %.1f s"), Best) : TEXT("")),
			FLinearColor::White, Right, Y, true);

		if (const ALevelTimer* LevelTimer = Timer.Get())
		{
			const float Left = LevelTimer->GetTimeLeft();
			const bool bWarn = Left <= LevelTimer->WarningThreshold;
			DrawLine(FString::Printf(TEXT("TEMPO RIMASTO: %.1f"), Left),
				bWarn ? FLinearColor(1.f, 0.2f, 0.15f) : FLinearColor(1.f, 0.9f, 0.2f), Right, Y, true);
		}

		const bool bDouble = Abilities ? Abilities->CanDoubleJump() : Run->HasAbility(EPlatformerAbility::DoubleJump);
		const bool bWall = Abilities ? Abilities->CanWallRun() : Run->HasAbility(EPlatformerAbility::WallRun);
		DrawLine(FString::Printf(TEXT("Doppio salto: %s"), bDouble ? TEXT("SI") : TEXT("da trovare")),
			bDouble ? FLinearColor(0.45f, 0.85f, 1.f) : FLinearColor(0.6f, 0.6f, 0.6f), Right, Y, true);
		DrawLine(FString::Printf(TEXT("Wall run / wall jump: %s"), bWall ? TEXT("SI") : TEXT("no")),
			bWall ? FLinearColor(0.45f, 0.85f, 1.f) : FLinearColor(0.6f, 0.6f, 0.6f), Right, Y, true);

		// le abilità nuove: jetpack (carburante), casse (cariche), torcia (batteria)
		if (Jetpack && Jetpack->IsUnlocked())
		{
			DrawLine(TEXT("Jetpack: tieni premuto salto in aria"), FLinearColor(1.f, 0.6f, 0.25f), Right, Y, true);
			DrawBar(Jetpack->GetFuelPercent(), FLinearColor(1.f, 0.55f, 0.15f), SW - 316.f, Y);
		}
		if (Crates && Crates->IsUnlocked())
		{
			const float Wait = Crates->GetRechargeTimeLeft();
			DrawLine(Wait > 0.f ? FString::Printf(TEXT("Casse [F]: ricarica %.1f s"), Wait)
				: FString::Printf(TEXT("Casse [F]: %d / %d"), Crates->GetCharges(), Crates->MaxCharges),
				Wait > 0.f ? FLinearColor(0.7f, 0.7f, 0.7f) : FLinearColor(0.85f, 0.65f, 0.4f), Right, Y, true);
		}
		if (Torch)
		{
			DrawLine(Torch->IsFirstPerson() ? TEXT("Torcia (rotellina indietro per uscire)") : TEXT("Torcia: rotellina avanti = prima persona"),
				FLinearColor(1.f, 0.95f, 0.5f), Right, Y, true);
			DrawBar(Torch->GetBatteryPercent(), FLinearColor(1.f, 0.92f, 0.35f), SW - 316.f, Y);
		}

		// barra della vita (il nostro componente)
		if (Health)
		{
			const float Pct = Health->MaxHealth > 0.f ? Health->GetHealth() / Health->MaxHealth : 0.f;
			DrawLine(FString::Printf(TEXT("Vita: %.0f"), Health->GetHealth()), FLinearColor(0.5f, 1.f, 0.5f), Right, Y, true);
			DrawBar(Pct, FLinearColor(0.2f + 0.8f * (1.f - Pct), 0.85f * Pct + 0.1f, 0.2f, 1.f), SW - 316.f, Y);
		}
		PanelHeight = Y - 14.f + 6.f;   // il riquadro scuro si adatta alle righe (dal prossimo frame)
	}

	// ------------------------------------------------ mirino in prima persona
	if (Torch && Torch->IsFirstPerson())
	{
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.7f), SW * 0.5f - 2.f, SH * 0.5f - 2.f, 4.f, 4.f);
		if (Torch->GetBatteryPercent() <= 0.f)
		{
			float Y = SH * 0.58f;
			DrawLine(TEXT("Torcia scarica: cerca una batteria"), FLinearColor(1.f, 0.5f, 0.3f), SW * 0.5f - 160.f, Y, false);
		}
	}

	// ------------------------------------------------ wall run
	if (Abilities && Abilities->IsWallRunning())
	{
		float Y = SH * 0.62f;
		DrawLine(TEXT("WALL RUN  -  salta per staccarti"), FLinearColor(0.5f, 0.9f, 1.f), SW * 0.5f + 80.f, Y, false);
	}

	// ------------------------------------------------ "premi E" (UE12)
	if (Abilities)
	{
		if (AActor* Focus = Abilities->GetFocusedInteractable())
		{
			const FText Prompt = IPlatformerInteractable::Execute_GetInteractPrompt(Focus);
			const FString Text = FString::Printf(TEXT("[E]  %s"), *Prompt.ToString());
			UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
			float W = 0.f, H = 0.f;
			GetTextSize(Text, W, H, Font, TextScale);
			const float X = (SW - W) * 0.5f;
			const float Y = SH * 0.72f;
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.5f), X - 16.f, Y - 8.f, W + 32.f, H + 16.f);
			DrawText(Text, FLinearColor::White, X, Y, Font, TextScale);
		}
	}

	// ------------------------------------------------ messaggi
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	Messages.RemoveAll([Now](const FHudMessage& M) { return M.ExpireTime < Now; });
	float MY = SH * 0.2f;
	for (const FHudMessage& M : Messages)
	{
		UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
		float W = 0.f, H = 0.f;
		GetTextSize(M.Text, W, H, Font, TextScale);
		const float Fade = FMath::Clamp(M.ExpireTime - Now, 0.f, 1.f);
		FLinearColor C = M.Color;
		C.A = Fade;
		DrawText(M.Text, FLinearColor(0.f, 0.f, 0.f, 0.8f * Fade), (SW - W) * 0.5f + 2.f, MY + 2.f, Font, TextScale);
		DrawText(M.Text, C, (SW - W) * 0.5f, MY, Font, TextScale);
		MY += H + 8.f;
	}
}
