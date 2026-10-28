#pragma once

#include "CoreMinimal.h"
#include "Dialogue/KeyValues.h"

/** Sorte de ligne affichée dans la colonne de dialogue. */
enum class EDialogueLineKind : uint8
{
	Narration,	// texte du narrateur, en italique
	Skill,		// une compétence qui parle
	Character,	// un personnage
	Official,	// voix officielle (estafette, crieur)
	Player,		// la réplique choisie par le joueur
	Roll,		// résultat d'un jet de dés
	Gauge,		// santé / moral / expérience
	System		// fin de scène, erreurs
};

/** Une ligne prête à afficher. */
struct FDialogueLineView
{
	EDialogueLineKind Kind = EDialogueLineKind::Narration;
	FString Speaker;	// nom affiché, en majuscules pour les compétences
	FString Tag;		// ex. "[Moyen : réussite]" pour un test passif
	FString Text;
	FLinearColor Color = FLinearColor::White;
	int32 Die1 = 0;
	int32 Die2 = 0;
	bool bSuccess = false;
};

/** Un choix proposé au joueur dans le noeud courant. */
struct FDialogueChoiceView
{
	int32 Number = 0;		// 1, 2, 3... (touche du clavier)
	FString Text;
	FString CheckLabel;		// ex. "[Rhétorique · Moyen 10 · 58 %]"
	bool bRedCheck = false;
	bool bWhiteCheck = false;
	bool bDisabled = false;	// test blanc raté, à retenter plus tard
};

/** Description d'une compétence. */
struct FSkillInfo
{
	FName Id;
	FString Name;
	FString Attribute;
	FLinearColor Color;
};

/** Profil de départ. */
struct FArchetype
{
	FString Name;
	FString Description;
	TMap<FName, int32> Skills;
};

/** Une scène chargée depuis Content/Dialogues. */
struct FDialogueScene
{
	FName Id;
	FString Title;
	FString Start;
	FName Next;
	int32 Order = 0;
	TMap<FString, FKVPair> Nodes;
};
