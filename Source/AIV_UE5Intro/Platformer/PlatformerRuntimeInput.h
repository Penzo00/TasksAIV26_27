// Piccoli aiuti per l'Enhanced Input creato DA CODICE.
// Le abilità nuove (zoom della telecamera per la torcia, tasto delle casse)
// non hanno un asset IA_/IMC_ nel progetto: le azioni e il loro mapping
// vengono creati a runtime e aggiunti al giocatore con una priorità alta,
// senza toccare IMC_Default di Bot2. In Blueprint lo stesso si fa creando
// gli asset Input Action e Input Mapping Context e usando "Add Mapping Context".

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"

class APawn;
class UInputAction;
class UInputMappingContext;

namespace PlatformerRuntimeInput
{
	/** Crea un'azione di input (senza asset). */
	AIV_UE5INTRO_API UInputAction* MakeAction(UObject* Outer, FName Name, EInputActionValueType ValueType);

	/** Crea un mapping context con un'azione legata ai tasti indicati. */
	AIV_UE5INTRO_API UInputMappingContext* MakeContext(UObject* Outer, UInputAction* Action, const TArray<FKey>& Keys);

	/** Aggiunge il context al giocatore che controlla il pawn. Falso se il pawn non ha ancora un PlayerController. */
	AIV_UE5INTRO_API bool AddContext(const APawn* Pawn, UInputMappingContext* Context, int32 Priority = 5);
}
