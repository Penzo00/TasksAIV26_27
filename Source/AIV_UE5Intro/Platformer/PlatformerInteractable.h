// UE12 - L'interfaccia di interazione: qualunque actor che la implementa
// risponde al tasto E (leve, cartelli, interruttori...).
// È la Blueprint Interface della lezione, scritta in C++: la si può
// implementare anche in un Blueprint (Class Settings > Interfaces).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlatformerInteractable.generated.h"

class ACharacter;

UINTERFACE(MinimalAPI, Blueprintable)
class UPlatformerInteractable : public UInterface
{
	GENERATED_BODY()
};

class AIV_UE5INTRO_API IPlatformerInteractable
{
	GENERATED_BODY()

public:
	/** Il personaggio ha premuto E guardando questo oggetto. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interazione")
	void Interact(ACharacter* ByCharacter);

	/** Il testo che l'HUD mostra accanto al tasto, per esempio "Tira la leva". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interazione")
	FText GetInteractPrompt() const;
};
