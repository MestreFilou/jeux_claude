#include "Game/ThermidorGameMode.h"
#include "Game/Commissaire.h"
#include "Game/ThermidorPlayerController.h"
#include "Game/ThermidorSets.h"
#include "Dialogue/DialogueSubsystem.h"
#include "HuitThermidor.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AThermidorGameMode::AThermidorGameMode()
{
	DefaultPawnClass = ACommissaire::StaticClass();
	PlayerControllerClass = AThermidorPlayerController::StaticClass();
}

void AThermidorGameMode::StartPlay()
{
	Super::StartPlay();

	UDialogueSubsystem* Dialogue = GetGameInstance()->GetSubsystem<UDialogueSubsystem>();
	if (!Dialogue) { return; }

	SceneHandle = Dialogue->OnSceneStarted.AddUObject(this, &AThermidorGameMode::BuildSet);
	MusicHandle = Dialogue->OnMusic.AddUObject(this, &AThermidorGameMode::PlayMusic);

	// Derrière l'écran de départ, on montre déjà la salle du comité.
	BuildSet(Dialogue->GetFirstSceneId());
}

void AThermidorGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDialogueSubsystem* Dialogue = GI->GetSubsystem<UDialogueSubsystem>())
		{
			Dialogue->OnSceneStarted.Remove(SceneHandle);
			Dialogue->OnMusic.Remove(MusicHandle);
		}
	}
	Super::EndPlay(Reason);
}

void AThermidorGameMode::RestartPlayer(AController* NewPlayer)
{
	// La carte vide n'a pas de point de départ : on place le joueur dans le décor courant.
	Super::RestartPlayer(NewPlayer);
	if (NewPlayer && NewPlayer->GetPawn())
	{
		NewPlayer->GetPawn()->TeleportTo(LastStart, LastRotation);
		NewPlayer->SetControlRotation(LastRotation);
	}
}

void AThermidorGameMode::ClearSet()
{
	for (AActor* A : SpawnedSet)
	{
		if (IsValid(A)) { A->Destroy(); }
	}
	SpawnedSet.Reset();
}

void AThermidorGameMode::BuildSet(FName SceneId)
{
	if (SceneId == BuiltScene && SpawnedSet.Num() > 0)
	{
		return;	// déjà construit (écran de départ puis début de partie)
	}
	ClearSet();
	BuiltScene = SceneId;

	const UDialogueSubsystem* Dialogue = GetGameInstance()->GetSubsystem<UDialogueSubsystem>();
	TArray<AActor*> Spawned;
	ThermidorSets::FSetResult Result;
	if (SceneId == TEXT("scene01"))
	{
		Result = ThermidorSets::BuildComite(GetWorld(), Spawned);
	}
	else if (SceneId == TEXT("scene02"))
	{
		Result = ThermidorSets::BuildPlace(GetWorld(), Spawned, Dialogue);
	}
	else if (SceneId == TEXT("scene03_cave"))
	{
		Result = ThermidorSets::BuildCave(GetWorld(), Spawned, Dialogue);
	}
	else if (SceneId == TEXT("scene03_comite"))
	{
		Result = ThermidorSets::BuildBureauVadier(GetWorld(), Spawned);
	}
	else if (SceneId == TEXT("scene04"))
	{
		Result = ThermidorSets::BuildConvention(GetWorld(), Spawned);
	}
	else if (SceneId == TEXT("scene05"))
	{
		Result = ThermidorSets::BuildGreve(GetWorld(), Spawned, Dialogue);
	}
	else
	{
		UE_LOG(LogThermidor, Warning, TEXT("Pas de décor pour la scène %s : on garde le vide."), *SceneId.ToString());
	}
	for (AActor* A : Spawned) { SpawnedSet.Add(A); }
	LastStart = Result.PlayerStart;
	LastRotation = Result.PlayerRotation;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->TeleportTo(Result.PlayerStart, Result.PlayerRotation);
		}
		PC->SetControlRotation(Result.PlayerRotation);
	}
}

void AThermidorGameMode::PlayMusic(FName MusicId)
{
	// Les pistes s'importent dans Content/Audio, avec le point remplacé par un tiret bas :
	// Musique.Reveil -> /Game/Audio/Musique_Reveil
	const FString AssetName = MusicId.ToString().Replace(TEXT("."), TEXT("_"));
	const FString Path = FString::Printf(TEXT("/Game/Audio/%s.%s"), *AssetName, *AssetName);
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);

	if (Music)
	{
		Music->FadeOut(2.f, 0.f);
		Music = nullptr;
	}
	if (!Sound)
	{
		UE_LOG(LogThermidor, Log, TEXT("Musique %s : pas encore de fichier (%s)"), *MusicId.ToString(), *Path);
		return;
	}
	Music = UGameplayStatics::CreateSound2D(this, Sound, 0.8f, 1.f, 0.f, nullptr, true, false);
	if (Music)
	{
		Music->FadeIn(2.f, 0.8f);
	}
}
