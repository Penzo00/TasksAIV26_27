#include "PlatformerMenu.h"

#include "PlatformerRunSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Misc/PackageName.h"

namespace
{
	const FName BoxCpp(TEXT("Menu_Cpp"));
	const FName BoxBlueprint(TEXT("Menu_Blueprint"));
}

// ---------------------------------------------------------------------
// GameMode: nessun personaggio, mouse visibile
// ---------------------------------------------------------------------

APlatformerMenuGameMode::APlatformerMenuGameMode()
{
	DefaultPawnClass = nullptr;   // nel menu non serve un personaggio: la vista è la CameraActor del livello
	HUDClass = APlatformerMenuHUD::StaticClass();
}

void APlatformerMenuGameMode::StartPlay()
{
	Super::StartPlay();
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;       // servono alle hit box dell'HUD
		PC->bEnableMouseOverEvents = true;
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

// ---------------------------------------------------------------------
// HUD: il menu
// ---------------------------------------------------------------------

bool APlatformerMenuHUD::BlueprintLevelExists() const
{
	return FPackageName::DoesPackageExist(MapsFolder + BlueprintFirstLevel.ToString());
}

void APlatformerMenuHUD::DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color,
	float Scale, bool bLarge)
{
	UFont* Font = GEngine ? (bLarge ? GEngine->GetLargeFont() : GEngine->GetMediumFont()) : nullptr;
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, Font, Scale);
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.85f), CenterX - W * 0.5f + 2.f, Y + 2.f, Font, Scale);
	DrawText(Text, Color, CenterX - W * 0.5f, Y, Font, Scale);
}

void APlatformerMenuHUD::DrawButton(int32 Index, const FString& Title, const FString& Line1, const FString& Line2,
	float X, float Y, float W, float H, const FLinearColor& Accent)
{
	const bool bSel = (Selected == Index);
	const float Border = bSel ? 5.f : 2.f;
	const FLinearColor Edge = bSel ? Accent : FLinearColor(0.4f, 0.4f, 0.45f, 0.9f);
	DrawRect(Edge, X - Border, Y - Border, W + Border * 2.f, H + Border * 2.f);
	DrawRect(FLinearColor(0.04f, 0.045f, 0.06f, bSel ? 0.92f : 0.8f), X, Y, W, H);
	DrawRect(Accent * FLinearColor(1.f, 1.f, 1.f, bSel ? 1.f : 0.55f), X, Y, W, 10.f);

	const float S = FMath::Clamp(H / 260.f, 0.7f, 1.6f);
	DrawCentered(Title, X + W * 0.5f, Y + H * 0.16f, bSel ? FLinearColor::White : FLinearColor(0.8f, 0.8f, 0.85f), 2.2f * S, true);
	DrawCentered(Line1, X + W * 0.5f, Y + H * 0.56f, FLinearColor(0.85f, 0.85f, 0.9f), 1.1f * S, false);
	DrawCentered(Line2, X + W * 0.5f, Y + H * 0.72f, Accent, 1.1f * S, false);

	// la hit box si ridisegna a ogni frame: il click del mouse arriva in NotifyHitBoxClick
	AddHitBox(FVector2D(X, Y), FVector2D(W, H), Index == 0 ? BoxCpp : BoxBlueprint, true);
}

void APlatformerMenuHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	HandleKeys();

	const float SW = Canvas->ClipX;
	const float SH = Canvas->ClipY;
	const float Now = GetWorld() ? static_cast<float>(GetWorld()->GetRealTimeSeconds()) : 0.f;

	// velo scuro sotto e sopra, per leggere bene sopra la scena del livello
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.45f), 0.f, 0.f, SW, SH * 0.3f);
	DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.35f), 0.f, SH * 0.82f, SW, SH * 0.18f);

	const float TitleScale = FMath::Clamp(SH / 720.f, 0.8f, 2.f);
	DrawCentered(TEXT("AIV PLATFORMER"), SW * 0.5f, SH * 0.08f, FLinearColor(1.f, 0.78f, 0.35f), 3.f * TitleScale, true);
	DrawCentered(TEXT("Scegli la versione dei tre livelli"), SW * 0.5f, SH * 0.2f, FLinearColor::White, 1.4f * TitleScale, false);

	const float BW = FMath::Min(SW * 0.34f, 560.f);
	const float BH = FMath::Min(SH * 0.36f, 300.f);
	const float Gap = SW * 0.05f;
	const float Y = SH * 0.38f;
	const float X0 = SW * 0.5f - Gap * 0.5f - BW;
	const float X1 = SW * 0.5f + Gap * 0.5f;

	DrawButton(0, TEXT("C++"), TEXT("i livelli costruiti con le classi C++"), CppFirstLevel.ToString(),
		X0, Y, BW, BH, FLinearColor(0.25f, 0.6f, 1.f));
	const bool bBpOk = BlueprintLevelExists();
	DrawButton(1, TEXT("Blueprints"), TEXT("la stessa cosa, rifatta a nodi"),
		bBpOk ? BlueprintFirstLevel.ToString() : BlueprintFirstLevel.ToString() + TEXT("  (non ancora creato)"),
		X1, Y, BW, BH, bBpOk ? FLinearColor(0.35f, 0.9f, 0.45f) : FLinearColor(0.9f, 0.5f, 0.2f));

	DrawCentered(TEXT("clic, oppure  1 / C = C++    2 / B = Blueprints    frecce + Invio"),
		SW * 0.5f, SH * 0.86f, FLinearColor(0.85f, 0.85f, 0.85f), 1.f * TitleScale, false);
	DrawCentered(TEXT("nei livelli: M torna a questo menu"), SW * 0.5f, SH * 0.91f,
		FLinearColor(0.7f, 0.7f, 0.7f), 0.9f * TitleScale, false);

	if (!Notice.IsEmpty() && Now - NoticeTime < 4.f)
	{
		const float A = FMath::Clamp(4.f - (Now - NoticeTime), 0.f, 1.f);
		DrawCentered(Notice, SW * 0.5f, SH * 0.76f, FLinearColor(1.f, 0.55f, 0.3f, A), 1.2f * TitleScale, false);
	}
}

void APlatformerMenuHUD::HandleKeys()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || bChosen)
	{
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::One) || PC->WasInputKeyJustPressed(EKeys::C))
	{
		Choose(0);
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Two) || PC->WasInputKeyJustPressed(EKeys::B))
	{
		Choose(1);
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Left) || PC->WasInputKeyJustPressed(EKeys::A)
		|| PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Left) || PC->WasInputKeyJustPressed(EKeys::Gamepad_LeftStick_Left))
	{
		Selected = 0;
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Right) || PC->WasInputKeyJustPressed(EKeys::D)
		|| PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Right) || PC->WasInputKeyJustPressed(EKeys::Gamepad_LeftStick_Right))
	{
		Selected = 1;
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Enter) || PC->WasInputKeyJustPressed(EKeys::SpaceBar)
		|| PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
	{
		Choose(Selected);
	}
}

void APlatformerMenuHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	if (BoxName == BoxCpp)
	{
		Choose(0);
	}
	else if (BoxName == BoxBlueprint)
	{
		Choose(1);
	}
}

void APlatformerMenuHUD::NotifyHitBoxBeginCursorOver(FName BoxName)
{
	Super::NotifyHitBoxBeginCursorOver(BoxName);
	Selected = (BoxName == BoxBlueprint) ? 1 : 0;
}

void APlatformerMenuHUD::NotifyHitBoxEndCursorOver(FName BoxName)
{
	Super::NotifyHitBoxEndCursorOver(BoxName);
}

void APlatformerMenuHUD::Choose(int32 Option)
{
	if (bChosen)
	{
		return;
	}
	Selected = FMath::Clamp(Option, 0, 1);
	const bool bBlueprints = (Selected == 1);
	if (bBlueprints && !BlueprintLevelExists())
	{
		Notice = FString::Printf(TEXT("%s non esiste ancora: crealo seguendo la guida (Docs/Blueprint_Livelli)"),
			*BlueprintFirstLevel.ToString());
		NoticeTime = GetWorld() ? static_cast<float>(GetWorld()->GetRealTimeSeconds()) : 0.f;
		return;
	}
	UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this);
	if (!Run)
	{
		return;
	}
	bChosen = true;
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	Run->StartMode(bBlueprints, bBlueprints ? BlueprintFirstLevel : CppFirstLevel);
}
