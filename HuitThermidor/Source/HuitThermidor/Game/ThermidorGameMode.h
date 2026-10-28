#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ThermidorGameMode.generated.h"

class UAudioComponent;

/**
 * Construit le décor de chaque scène par le code (aucune carte à créer)
 * et joue la musique demandée par les dialogues.
 */
UCLASS()
class HUITTHERMIDOR_API AThermidorGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AThermidorGameMode();
	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void RestartPlayer(AController* NewPlayer) override;

private:
	void BuildSet(FName SceneId);
	void ClearSet();
	void PlayMusic(FName MusicId);

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedSet;

	UPROPERTY()
	TObjectPtr<UAudioComponent> Music;

	FName BuiltScene;
	FVector LastStart = FVector(0.f, 0.f, 100.f);
	FRotator LastRotation = FRotator::ZeroRotator;
	FDelegateHandle SceneHandle;
	FDelegateHandle MusicHandle;
};
