// Il menu iniziale: si sceglie se giocare i livelli fatti in C++
// (L_GD13_Livello1..3) o la loro copia rifatta in Blueprint
// (L_GD13_Livello1_BP..3_BP, da costruire seguendo la guida in
// Docs/Blueprint_Livelli).
//
// Tutto è disegnato dall'HUD con il Canvas (come APlatformerHUD): due
// "pulsanti" cliccabili con il mouse (hit box dell'HUD) e scorciatoie da
// tastiera/pad: 1 o C = C++, 2 o B = Blueprints, frecce + Invio, pad
// sinistra/destra + A. Se il livello Blueprint non esiste ancora, il menu lo
// dice invece di aprire una mappa vuota.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "PlatformerMenu.generated.h"

UCLASS()
class AIV_UE5INTRO_API APlatformerMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APlatformerMenuGameMode();

	virtual void StartPlay() override;
};

UCLASS()
class AIV_UE5INTRO_API APlatformerMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** Primo livello della versione C++. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName CppFirstLevel = FName(TEXT("L_GD13_Livello1"));

	/** Primo livello della versione Blueprint (nome consigliato: quello C++ con _BP in fondo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName BlueprintFirstLevel = FName(TEXT("L_GD13_Livello1_BP"));

	/** Cartella dei livelli, per controllare che il livello Blueprint esista. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FString MapsFolder = TEXT("/Game/Maps/");

	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;
	virtual void NotifyHitBoxBeginCursorOver(FName BoxName) override;
	virtual void NotifyHitBoxEndCursorOver(FName BoxName) override;

	/** Sceglie la modalità: 0 = C++, 1 = Blueprints. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void Choose(int32 Option);

private:
	bool BlueprintLevelExists() const;
	void HandleKeys();
	void DrawButton(int32 Index, const FString& Title, const FString& Line1, const FString& Line2,
		float X, float Y, float W, float H, const FLinearColor& Accent);
	void DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, bool bLarge);

	int32 Selected = 0;
	bool bHoverFromMouse = false;
	bool bChosen = false;
	FString Notice;
	float NoticeTime = -100.f;
};
