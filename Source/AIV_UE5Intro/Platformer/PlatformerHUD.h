// UE11 - L'interfaccia a schermo. Qui è un HUD disegnato in C++ (Canvas):
// monete, morti, tempo, record, vita, abilità sbloccate, il "premi E"
// dell'interazione e i messaggi. Si vede anche nelle build Shipping
// (a differenza dei Print String / On Screen Debug Message).
// La barra di vita e stamina di BP_Bot2 (WB_HUD1) resta: questo pannello sta
// a destra per non coprirla.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PlatformerHUD.generated.h"

class ALevelTimer;

UCLASS()
class AIV_UE5INTRO_API APlatformerHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** Mostra il pannello in alto a destra. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	bool bShowPanel = true;

	/** Scala del testo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD", meta = (ClampMin = "0.5"))
	float TextScale = 1.f;

	/** Secondi in cui un messaggio resta a schermo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD", meta = (ClampMin = "0.5"))
	float MessageDuration = 3.5f;

	/** Aggiunge un messaggio al centro dello schermo. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowMessage(const FString& Message, FLinearColor Color);

	virtual void DrawHUD() override;

protected:
	virtual void BeginPlay() override;

private:
	struct FHudMessage
	{
		FString Text;
		FLinearColor Color;
		float ExpireTime = 0.f;
	};

	void DrawLine(const FString& Text, const FLinearColor& Color, float X, float& Y, bool bRightAligned);
	void DrawBar(float Percent, const FLinearColor& Color, float X, float& Y);
	float PanelHeight = 300.f;

	TArray<FHudMessage> Messages;
	TWeakObjectPtr<ALevelTimer> Timer;
};
