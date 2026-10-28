#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dialogue/DialogueTypes.h"
#include "DialogueSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueLine, const FDialogueLineView&);
DECLARE_MULTICAST_DELEGATE(FOnDialogueChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueScene, FName /*SceneId*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueMusic, FName /*MusicId*/);

/**
 * Le moteur de dialogue : charge les scènes, garde l'état du commissaire
 * (compétences, jauges, expérience, drapeaux) et joue les noeuds.
 * Les règles sont les mêmes que dans le prototype navigateur.
 */
UCLASS()
class HUITTHERMIDOR_API UDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Relit tous les fichiers de Content/Dialogues. */
	void LoadScenes();

	// --- Partie ---
	const TArray<FArchetype>& GetArchetypes() const { return Archetypes; }
	void NewGame(int32 ArchetypeIndex);
	bool StartScene(FName SceneId);
	bool IsInGame() const { return bInGame; }
	/** Revient à l'écran de départ (après une fin de partie). */
	void ReturnToTitle();
	FName GetFirstSceneId() const;

	// --- Joueur ---
	/** Choix par son numéro affiché (1, 2, 3...). */
	void ChooseNumber(int32 Number);
	/** Choix déclenché depuis le monde : le premier choix visible qui mène à ce noeud. */
	bool ChooseByTarget(const FString& TargetNode);
	/** Le noeud courant propose-t-il un choix qui mène à ce noeud ? */
	bool HasChoiceForTarget(const FString& TargetNode) const;
	void SpendPoint(FName SkillId);

	// --- Lecture de l'état ---
	static const TArray<FSkillInfo>& GetSkillInfos();
	int32 GetSkill(FName SkillId) const { return Skills.FindRef(SkillId); }
	int32 GetHealth() const { return Health; }
	int32 GetMaxHealth() const { return MaxHealth; }
	int32 GetMorale() const { return Morale; }
	int32 GetMaxMorale() const { return MaxMorale; }
	int32 GetXp() const { return Xp; }
	int32 GetXpPerPoint() const { return XpPerPoint; }
	int32 GetPoints() const { return Points; }
	bool IsGameOver() const { return bGameOver; }
	bool IsSceneEnded() const { return bSceneEnded; }
	bool IsAtHub() const;
	FName GetNextSceneId() const { return NextScene; }
	FName GetCurrentSceneId() const { return CurrentScene; }
	FString GetCurrentSceneTitle() const;
	FString GetArchetypeName() const { return ArchetypeName; }
	FName GetCurrentMusic() const { return CurrentMusic; }
	FString DescribeMusic(FName MusicId) const;
	const TArray<FDialogueLineView>& GetLog() const { return Log; }
	const TArray<FDialogueChoiceView>& GetChoices() const { return ChoiceViews; }
	bool HasFlag(const FString& Flag) const { return Flags.Contains(Flag); }
	TArray<FString> GetMeaningfulFlags() const;

	/** Chance de réussite en % d'un test actif (2d6 + niveau >= difficulté, double 6 / double 1). */
	static int32 ChanceOfSuccess(int32 Level, int32 Difficulty);
	static FString DifficultyName(int32 Difficulty);

	// --- Événements pour l'interface et le décor ---
	FOnDialogueLine OnLine;
	FOnDialogueChanged OnChanged;
	FOnDialogueScene OnSceneStarted;
	FOnDialogueChanged OnSceneEnded;
	FOnDialogueMusic OnMusic;

private:
	void Enter(const FString& NodeId);
	void Say(const FKVPair& Line);
	void AddLine(const FDialogueLineView& View);
	void ChangeGauge(bool bHealth, int32 Delta);
	void GainXp(int32 Amount);
	void RebuildChoices();
	void ChooseInternal(int32 ChoiceIndex);
	bool Passes(const FKVPair& Pair) const;
	FString ChoiceKey(int32 ChoiceIndex) const;
	const FKVPair* GetCurrentNode() const;
	const FSkillInfo* FindSkill(FName Id) const;

	TMap<FName, FDialogueScene> Scenes;
	TArray<FArchetype> Archetypes;
	TMap<FString, TPair<FString, EDialogueLineKind>> Speakers;
	TMap<FName, FString> MusicDescriptions;

	// État de la partie
	bool bInGame = false;
	bool bGameOver = false;
	bool bSceneEnded = false;
	FString ArchetypeName;
	TMap<FName, int32> Skills;
	TSet<FString> Flags;
	TSet<FString> Tried;
	TMap<FString, int32> FailedWhite;	// clé du choix -> niveau au moment de l'échec
	int32 Health = 0, MaxHealth = 0, Morale = 0, MaxMorale = 0;
	int32 Xp = 0, Points = 0;
	static constexpr int32 XpPerPoint = 3;
	FName CurrentScene;
	FString CurrentNode;
	FName NextScene;
	FName CurrentMusic;
	TArray<FDialogueLineView> Log;
	TArray<FDialogueChoiceView> ChoiceViews;
	TArray<int32> VisibleChoiceIndices;	// numéro affiché - 1 -> index du choix dans le noeud
};
