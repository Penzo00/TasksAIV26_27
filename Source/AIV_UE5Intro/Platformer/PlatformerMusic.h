#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "PlatformerMusic.generated.h"

class UAudioComponent;
class USoundBase;

/** Tiene la musica di sottofondo per tutta la partita (sopravvive ai cambi di livello). */
UCLASS()
class AIV_UE5INTRO_API UPlatformerMusicSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Fa partire la musica se non sta già suonando. */
	void EnsurePlaying(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "Platformer|Musica")
	void StopMusic();

	UFUNCTION(BlueprintCallable, Category = "Platformer|Musica")
	void SetMusicVolume(float NewVolume);

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> Music;

	UPROPERTY()
	TObjectPtr<USoundBase> Track;

	float Volume = 0.6f;
};

/** Esiste in ogni livello di gioco: al BeginPlay chiede di far partire la musica. */
UCLASS()
class AIV_UE5INTRO_API UPlatformerMusicStarter : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};