#pragma once

#include "CoreMinimal.h"

class UWorld;
class AActor;
class AInteractif;
class UDialogueSubsystem;

/**
 * Décors construits avec les formes de base du moteur (cubes, cylindres, sphères).
 * Ce sont des maquettes : on les remplacera par de vrais modèles plus tard,
 * en gardant les mêmes noms d'interactifs et de noeuds.
 */
namespace ThermidorSets
{
	struct FSetResult
	{
		FVector PlayerStart = FVector::ZeroVector;
		FRotator PlayerRotation = FRotator::ZeroRotator;
	};

	FSetResult BuildComite(UWorld* World, TArray<AActor*>& Out);
	FSetResult BuildPlace(UWorld* World, TArray<AActor*>& Out, const UDialogueSubsystem* Dialogue);
	FSetResult BuildCave(UWorld* World, TArray<AActor*>& Out, const UDialogueSubsystem* Dialogue);
	FSetResult BuildBureauVadier(UWorld* World, TArray<AActor*>& Out);
	FSetResult BuildConvention(UWorld* World, TArray<AActor*>& Out);
	FSetResult BuildGreve(UWorld* World, TArray<AActor*>& Out, const UDialogueSubsystem* Dialogue);
}
