#include "PlatformerGameMode.h"

#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

APlatformerGameMode::APlatformerGameMode()
{
	// Il personaggio controllabile è il Blueprint di Bot2.
	// Se lo spostate o rinominate, cambiate questo percorso
	// (oppure create un Blueprint figlio di questa GameMode e cambiate lì Default Pawn Class).
	static ConstructorHelpers::FClassFinder<APawn> Bot2(TEXT("/Game/Characters/Bot2/BP_Bot2"));
	if (Bot2.Succeeded())
	{
		DefaultPawnClass = Bot2.Class;
	}
}
