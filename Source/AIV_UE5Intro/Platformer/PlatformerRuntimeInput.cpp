#include "PlatformerRuntimeInput.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

UInputAction* PlatformerRuntimeInput::MakeAction(UObject* Outer, FName Name, EInputActionValueType ValueType)
{
	UInputAction* Action = NewObject<UInputAction>(Outer, Name);
	Action->ValueType = ValueType;
	return Action;
}

UInputMappingContext* PlatformerRuntimeInput::MakeContext(UObject* Outer, UInputAction* Action, const TArray<FKey>& Keys)
{
	UInputMappingContext* Context = NewObject<UInputMappingContext>(Outer);
	for (const FKey& Key : Keys)
	{
		Context->MapKey(Action, Key);
	}
	return Context;
}

bool PlatformerRuntimeInput::AddContext(const APawn* Pawn, UInputMappingContext* Context, int32 Priority)
{
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (!LocalPlayer || !Context)
	{
		return false;
	}
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return false;
	}
	Subsystem->AddMappingContext(Context, Priority);
	return true;
}
