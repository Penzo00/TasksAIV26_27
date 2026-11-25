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
	// passo 1
	const FName BoxCpp(TEXT("Menu_Cpp"));
	const FName BoxBlueprint(TEXT("Menu_Blueprint"));
	// passo 2
	const FName BoxNew(TEXT("Menu_Nuova"));
	const FName BoxContinue(TEXT("Menu_Continua"));
	const FName BoxBack(TEXT("Menu_Indietro"));

	/** Nome da mostrare per un'abilità salvata (quelle sconosciute, per esempio dei Blueprint, restano come sono). */
	FString AbilityLabel(FName Ability)
	{
		if (Ability == UPlatformerRunSubsystem::AbilityToName(EPlatformerAbility::DoubleJump))   return TEXT("doppio salto");
		if (Ability == UPlatformerRunSubsystem::AbilityToName(EPlatformerAbility::WallRun))      return TEXT("wall run");
		if (Ability == UPlatformerRunSubsystem::AbilityToName(EPlatformerAbility::Jetpack))      return TEXT("jetpack");
		if (Ability == UPlatformerRunSubsystem::AbilityToName(EPlatformerAbility::CrateSpawner)) return TEXT("generatore di casse");
		return Ability.ToString();
	}

	/** "L_GD13_Livello2_BP" -> "livello 2" quando il nome segue lo schema, altrimenti il nome della mappa. */
	FString LevelLabel(FName Level)
	{
		FString Short = FPackageName::GetShortName(Level.ToString());
		int32 At = Short.Find(TEXT("Livello"), ESearchCase::IgnoreCase);
		if (At != INDEX_NONE && At + 7 < Short.Len() && FChar::IsDigit(Short[At + 7]))
		{
			return FString::Printf(TEXT("livello %c"), Short[At + 7]);
		}
		return Short;
	}
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

void APlatformerMenuHUD::DrawButton(int32 Index, FName BoxName, bool bEnabled, const FString& Title,
	const TArray<FString>& Lines, float X, float Y, float W, float H, const FLinearColor& Accent)
{
	const bool bSel = (Selected == Index);
	const FLinearColor Acc = bEnabled ? Accent : FLinearColor(0.45f, 0.45f, 0.48f);
	const float Border = bSel ? 5.f : 2.f;
	const FLinearColor Edge = bSel ? Acc : FLinearColor(0.4f, 0.4f, 0.45f, 0.9f);
	DrawRect(Edge, X - Border, Y - Border, W + Border * 2.f, H + Border * 2.f);
	DrawRect(FLinearColor(0.04f, 0.045f, 0.06f, bSel ? 0.92f : 0.8f), X, Y, W, H);
	DrawRect(Acc * FLinearColor(1.f, 1.f, 1.f, bSel ? 1.f : 0.55f), X, Y, W, 10.f);

	const float S = FMath::Clamp(H / 260.f, 0.7f, 1.6f);
	const FLinearColor TitleColor = !bEnabled ? FLinearColor(0.55f, 0.55f, 0.58f)
		: (bSel ? FLinearColor::White : FLinearColor(0.8f, 0.8f, 0.85f));
	DrawCentered(Title, X + W * 0.5f, Y + H * 0.12f, TitleColor, 2.2f * S, true);

	// le righe di spiegazione; l'ultima prende il colore del pulsante
	const float LineStep = H * 0.13f;
	float LineY = Y + H * 0.44f;
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		const bool bLast = (i == Lines.Num() - 1);
		const FLinearColor C = bLast ? Acc : (bEnabled ? FLinearColor(0.88f, 0.88f, 0.92f) : FLinearColor(0.6f, 0.6f, 0.62f));
		DrawCentered(Lines[i], X + W * 0.5f, LineY, C, 1.05f * S, false);
		LineY += LineStep;
	}

	// la hit box si ridisegna a ogni frame: il click del mouse arriva in NotifyHitBoxClick
	AddHitBox(FVector2D(X, Y), FVector2D(W, H), BoxName, true);
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

	const float UIScale = FMath::Clamp(SH / 720.f, 0.8f, 2.f);
	DrawCentered(TEXT("AIV PLATFORMER"), SW * 0.5f, SH * 0.08f, FLinearColor(1.f, 0.78f, 0.35f), 3.f * UIScale, true);

	if (Step == 0)
	{
		DrawModeStep(SW, SH, UIScale);
	}
	else
	{
		DrawGameStep(SW, SH, UIScale);
	}

	if (!Notice.IsEmpty() && Now - NoticeTime < 4.f)
	{
		const float A = FMath::Clamp(4.f - (Now - NoticeTime), 0.f, 1.f);
		DrawCentered(Notice, SW * 0.5f, SH * 0.77f, FLinearColor(1.f, 0.55f, 0.3f, A), 1.2f * UIScale, false);
	}
}

void APlatformerMenuHUD::DrawModeStep(float SW, float SH, float UIScale)
{
	DrawCentered(TEXT("Scegli la versione dei tre livelli"), SW * 0.5f, SH * 0.2f, FLinearColor::White, 1.4f * UIScale, false);

	const float BW = FMath::Min(SW * 0.34f, 560.f);
	const float BH = FMath::Min(SH * 0.36f, 300.f);
	const float Gap = SW * 0.05f;
	const float Y = SH * 0.38f;
	const float X0 = SW * 0.5f - Gap * 0.5f - BW;
	const float X1 = SW * 0.5f + Gap * 0.5f;

	DrawButton(0, BoxCpp, true, TEXT("C++"),
		{ TEXT("i livelli costruiti con le classi C++"), TEXT("si gioca con Bot2"), CppFirstLevel.ToString() },
		X0, Y, BW, BH, FLinearColor(0.25f, 0.6f, 1.f));

	const bool bBpOk = BlueprintLevelExists();
	DrawButton(1, BoxBlueprint, bBpOk, TEXT("Blueprints"),
		{ TEXT("la stessa cosa, rifatta a nodi"), TEXT("si gioca con BP_Hero"),
		  bBpOk ? BlueprintFirstLevel.ToString() : BlueprintFirstLevel.ToString() + TEXT("  (non ancora creato)") },
		X1, Y, BW, BH, bBpOk ? FLinearColor(0.35f, 0.9f, 0.45f) : FLinearColor(0.9f, 0.5f, 0.2f));

	DrawCentered(TEXT("clic, oppure  1 / C = C++    2 / B = Blueprints    frecce + Invio"),
		SW * 0.5f, SH * 0.86f, FLinearColor(0.85f, 0.85f, 0.85f), 1.f * UIScale, false);
	DrawCentered(TEXT("nei livelli: M torna a questo menu"), SW * 0.5f, SH * 0.91f,
		FLinearColor(0.7f, 0.7f, 0.7f), 0.9f * UIScale, false);
}

void APlatformerMenuHUD::DrawGameStep(float SW, float SH, float UIScale)
{
	const FString Mode = bModeBlueprints ? TEXT("Blueprints con BP_Hero") : TEXT("C++ con Bot2");
	DrawCentered(FString::Printf(TEXT("Modalità: %s  -  come vuoi giocare?"), *Mode), SW * 0.5f, SH * 0.2f,
		FLinearColor::White, 1.4f * UIScale, false);

	const float BW = FMath::Min(SW * 0.36f, 600.f);
	const float BH = FMath::Min(SH * 0.38f, 320.f);
	const float Gap = SW * 0.05f;
	const float Y = SH * 0.36f;
	const float X0 = SW * 0.5f - Gap * 0.5f - BW;
	const float X1 = SW * 0.5f + Gap * 0.5f;

	// --- Nuova partita: cosa succede, detto chiaro
	TArray<FString> NewLines = {
		TEXT("Si comincia dal livello 1,"),
		TEXT("SENZA potenziamenti e con zero monete:"),
		TEXT("le abilità vanno sbloccate di nuovo."),
		bHasSave ? TEXT("Sostituisce il salvataggio attuale (i record restano)")
		         : TEXT("Crea il primo salvataggio di questa modalità")
	};
	DrawButton(0, BoxNew, true, TEXT("Nuova partita"), NewLines, X0, Y, BW, BH, FLinearColor(1.f, 0.6f, 0.25f));

	// --- Continua: da dove si riparte e cosa si conserva
	TArray<FString> ContinueLines;
	if (bHasSave)
	{
		FString Abilities;
		for (const FName& A : SavedAbilities)
		{
			if (!Abilities.IsEmpty())
			{
				Abilities += TEXT(", ");
			}
			Abilities += AbilityLabel(A);
		}
		const FName Where = SavedLevel.IsNone() ? (bModeBlueprints ? BlueprintFirstLevel : CppFirstLevel) : SavedLevel;
		ContinueLines.Add(FString::Printf(TEXT("Riprendi dal %s, l'ultimo raggiunto,"), *LevelLabel(Where)));
		ContinueLines.Add(TEXT("CONSERVANDO potenziamenti e monete."));
		ContinueLines.Add(Abilities.IsEmpty() ? FString(TEXT("Potenziamenti: nessuno ancora"))
		                                      : FString(TEXT("Potenziamenti: ")) + Abilities);
		ContinueLines.Add(FString::Printf(TEXT("Monete salvate: %d"), SavedCoins));
	}
	else
	{
		ContinueLines.Add(TEXT("Riprende dall'ultimo livello raggiunto"));
		ContinueLines.Add(TEXT("con potenziamenti e monete salvate."));
		ContinueLines.Add(TEXT(""));
		ContinueLines.Add(TEXT("Nessun salvataggio: scegli Nuova partita"));
	}
	DrawButton(1, BoxContinue, bHasSave, TEXT("Continua"), ContinueLines, X1, Y, BW, BH, FLinearColor(0.35f, 0.9f, 0.45f));

	// --- Indietro, piccolo, sotto i pulsanti
	const FString BackText = TEXT("<  Indietro (cambia modalità)");
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	float TW = 0.f, TH = 0.f;
	GetTextSize(BackText, TW, TH, Font, 1.f * UIScale);
	const float BX = SW * 0.5f - TW * 0.5f - 16.f;
	const float BY = SH * 0.79f;
	const bool bBackSel = (Selected == 2);
	DrawRect(bBackSel ? FLinearColor(0.9f, 0.9f, 0.95f, 0.9f) : FLinearColor(0.4f, 0.4f, 0.45f, 0.8f), BX - 2.f, BY - 2.f, TW + 36.f, TH + 16.f);
	DrawRect(FLinearColor(0.04f, 0.045f, 0.06f, 0.9f), BX, BY, TW + 32.f, TH + 12.f);
	DrawCentered(BackText, SW * 0.5f, BY + 6.f, bBackSel ? FLinearColor::White : FLinearColor(0.8f, 0.8f, 0.85f), 1.f * UIScale, false);
	AddHitBox(FVector2D(BX, BY), FVector2D(TW + 32.f, TH + 12.f), BoxBack, true);

	DrawCentered(TEXT("clic, oppure  1 / N = Nuova partita    2 / C = Continua    Esc = indietro"),
		SW * 0.5f, SH * 0.88f, FLinearColor(0.85f, 0.85f, 0.85f), 1.f * UIScale, false);
	DrawCentered(TEXT("il salvataggio è separato per ogni modalità"), SW * 0.5f, SH * 0.93f,
		FLinearColor(0.7f, 0.7f, 0.7f), 0.9f * UIScale, false);
}

void APlatformerMenuHUD::HandleKeys()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || bChosen)
	{
		return;
	}
	auto Pressed = [PC](const FKey& Key) { return PC->WasInputKeyJustPressed(Key); };
	const bool bLeft = Pressed(EKeys::Left) || Pressed(EKeys::A)
		|| Pressed(EKeys::Gamepad_DPad_Left) || Pressed(EKeys::Gamepad_LeftStick_Left);
	const bool bRight = Pressed(EKeys::Right) || Pressed(EKeys::D)
		|| Pressed(EKeys::Gamepad_DPad_Right) || Pressed(EKeys::Gamepad_LeftStick_Right);
	const bool bConfirm = Pressed(EKeys::Enter) || Pressed(EKeys::SpaceBar) || Pressed(EKeys::Gamepad_FaceButton_Bottom);

	if (Step == 0)
	{
		if (Pressed(EKeys::One) || Pressed(EKeys::C))       { Choose(0); }
		else if (Pressed(EKeys::Two) || Pressed(EKeys::B))  { Choose(1); }
		else if (bLeft)                                     { Selected = 0; }
		else if (bRight)                                    { Selected = 1; }
		else if (bConfirm)                                  { Choose(Selected); }
		return;
	}

	if (Pressed(EKeys::Escape) || Pressed(EKeys::BackSpace) || Pressed(EKeys::Gamepad_FaceButton_Right))
	{
		Back();
	}
	else if (Pressed(EKeys::One) || Pressed(EKeys::N))      { StartGame(false); }
	else if (Pressed(EKeys::Two) || Pressed(EKeys::C))      { StartGame(true); }
	else if (bLeft)                                         { Selected = 0; }
	else if (bRight)                                        { Selected = 1; }
	else if (Pressed(EKeys::Down) || Pressed(EKeys::S) || Pressed(EKeys::Gamepad_DPad_Down)) { Selected = 2; }
	else if (Pressed(EKeys::Up) || Pressed(EKeys::W) || Pressed(EKeys::Gamepad_DPad_Up))     { Selected = bHasSave ? 1 : 0; }
	else if (bConfirm)
	{
		if (Selected == 2) { Back(); }
		else               { StartGame(Selected == 1); }
	}
}

void APlatformerMenuHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	if (Step == 0)
	{
		if (BoxName == BoxCpp)            { Choose(0); }
		else if (BoxName == BoxBlueprint) { Choose(1); }
	}
	else
	{
		if (BoxName == BoxNew)            { StartGame(false); }
		else if (BoxName == BoxContinue)  { StartGame(true); }
		else if (BoxName == BoxBack)      { Back(); }
	}
}

void APlatformerMenuHUD::NotifyHitBoxBeginCursorOver(FName BoxName)
{
	Super::NotifyHitBoxBeginCursorOver(BoxName);
	if (BoxName == BoxCpp || BoxName == BoxNew)                 { Selected = 0; }
	else if (BoxName == BoxBlueprint || BoxName == BoxContinue) { Selected = 1; }
	else if (BoxName == BoxBack)                                { Selected = 2; }
}

void APlatformerMenuHUD::NotifyHitBoxEndCursorOver(FName BoxName)
{
	Super::NotifyHitBoxEndCursorOver(BoxName);
}

void APlatformerMenuHUD::ShowNotice(const FString& Text)
{
	Notice = Text;
	NoticeTime = GetWorld() ? static_cast<float>(GetWorld()->GetRealTimeSeconds()) : 0.f;
}

void APlatformerMenuHUD::RefreshSaveInfo()
{
	bHasSave = false;
	SavedLevel = NAME_None;
	SavedCoins = 0;
	SavedAbilities.Reset();
	if (const UPlatformerRunSubsystem* Run = UPlatformerRunSubsystem::Get(this))
	{
		bHasSave = Run->GetSaveSummary(bModeBlueprints, SavedLevel, SavedCoins, SavedAbilities);
	}
}

void APlatformerMenuHUD::Choose(int32 Option)
{
	if (bChosen || Step != 0)
	{
		return;
	}
	Selected = FMath::Clamp(Option, 0, 1);
	const bool bBlueprints = (Selected == 1);
	if (bBlueprints && !BlueprintLevelExists())
	{
		ShowNotice(FString::Printf(TEXT("%s non esiste ancora: crealo seguendo la guida (Docs/Blueprint_Livelli)"),
			*BlueprintFirstLevel.ToString()));
		return;
	}
	// passo 2: Nuova partita / Continua. Se c'è un salvataggio, è già selezionato "Continua".
	bModeBlueprints = bBlueprints;
	RefreshSaveInfo();
	Step = 1;
	Selected = bHasSave ? 1 : 0;
	Notice.Reset();
}

void APlatformerMenuHUD::Back()
{
	if (bChosen || Step == 0)
	{
		return;
	}
	Step = 0;
	Selected = bModeBlueprints ? 1 : 0;
	Notice.Reset();
}

void APlatformerMenuHUD::StartGame(bool bContinue)
{
	if (bChosen || Step != 1)
	{
		return;
	}
	if (bContinue && !bHasSave)
	{
		Selected = 0;
		ShowNotice(TEXT("Non c'è ancora un salvataggio per questa modalità: inizia una Nuova partita"));
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
	const FName First = bModeBlueprints ? BlueprintFirstLevel : CppFirstLevel;
	if (bContinue)
	{
		Run->ContinueGame(bModeBlueprints, First);
	}
	else
	{
		Run->NewGame(bModeBlueprints, First);
	}
}
