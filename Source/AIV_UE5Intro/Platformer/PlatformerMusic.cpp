#include "PlatformerMusic.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

namespace
{
	// Content/OST/GULHAM.uasset
	const TCHAR* MusicPath = TEXT("/Game/OST/GULHAM.GULHAM");
}

void UPlatformerMusicSubsystem::EnsurePlaying(UWorld* World)
{
	if (!World || (Music && Music->IsPlaying()))
	{
		return;   // sta già suonando: cambiare livello non la fa ripartire
	}
	if (!Track)
	{
		Track = LoadObject<USoundBase>(nullptr, MusicPath);
	}
	if (!Track)
	{
		UE_LOG(LogTemp, Warning, TEXT("Musica non trovata: %s"), MusicPath);
		return;
	}
	if (USoundWave* Wave = Cast<USoundWave>(Track))
	{
		Wave->bLooping = true;   // in loop anche se nell'asset non è spuntato
	}
	// 2D (si sente uguale ovunque), persistente fra un livello e l'altro, non si autodistrugge
	Music = UGameplayStatics::SpawnSound2D(World, Track, Volume, 1.f, 0.f, nullptr,
		/*bPersistAcrossLevelTransition*/ true, /*bAutoDestroy*/ false);
}

void UPlatformerMusicSubsystem::StopMusic()
{
	if (Music)
	{
		Music->Stop();
	}
}

void UPlatformerMusicSubsystem::SetMusicVolume(float NewVolume)
{
	Volume = FMath::Clamp(NewVolume, 0.f, 1.f);
	if (Music)
	{
		Music->SetVolumeMultiplier(Volume);
	}
}

void UPlatformerMusicSubsystem::Deinitialize()
{
	StopMusic();   // fine del Play nell'editor o chiusura del gioco
	Music = nullptr;
	Super::Deinitialize();
}

void UPlatformerMusicStarter::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UGameInstance* GI = InWorld.GetGameInstance())
	{
		if (UPlatformerMusicSubsystem* MusicSys = GI->GetSubsystem<UPlatformerMusicSubsystem>())
		{
			MusicSys->EnsurePlaying(&InWorld);
		}
	}
}