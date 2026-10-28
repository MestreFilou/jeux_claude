#include "Dialogue/DialogueSubsystem.h"
#include "HuitThermidor.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const FLinearColor ColIntellect(0.42f, 0.66f, 0.88f);
	const FLinearColor ColPsyche(0.65f, 0.55f, 0.88f);
	const FLinearColor ColPhysique(0.88f, 0.45f, 0.42f);
	const FLinearColor ColMotricite(0.88f, 0.77f, 0.35f);
	const FLinearColor ColText(0.89f, 0.85f, 0.78f);
	const FLinearColor ColOfficial(0.56f, 0.65f, 0.81f);
	const FLinearColor ColPlayer(0.91f, 0.76f, 0.48f);
	const FLinearColor ColMuted(0.58f, 0.53f, 0.47f);
	const FLinearColor ColGood(0.50f, 0.77f, 0.55f);
	const FLinearColor ColBad(0.88f, 0.45f, 0.42f);
}

const TArray<FSkillInfo>& UDialogueSubsystem::GetSkillInfos()
{
	static const TArray<FSkillInfo> Infos = {
		{ TEXT("logique"),    TEXT("Logique"),    TEXT("Intellect"), ColIntellect },
		{ TEXT("rhetorique"), TEXT("Rhétorique"), TEXT("Intellect"), ColIntellect },
		{ TEXT("empathie"),   TEXT("Empathie"),   TEXT("Psyché"),    ColPsyche },
		{ TEXT("volonte"),    TEXT("Volonté"),    TEXT("Psyché"),    ColPsyche },
		{ TEXT("ferveur"),    TEXT("Ferveur"),    TEXT("Psyché"),    ColPsyche },
		{ TEXT("spectres"),   TEXT("Spectres"),   TEXT("Psyché"),    ColPsyche },
		{ TEXT("endurance"),  TEXT("Endurance"),  TEXT("Physique"),  ColPhysique },
		{ TEXT("poigne"),     TEXT("Poigne"),     TEXT("Physique"),  ColPhysique },
		{ TEXT("perception"), TEXT("Perception"), TEXT("Motricité"), ColMotricite },
		{ TEXT("sangfroid"),  TEXT("Sang-froid"), TEXT("Motricité"), ColMotricite },
	};
	return Infos;
}

void UDialogueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	struct FLevel { const TCHAR* Id; int32 Level; };
	auto MakeArch = [](const TCHAR* Name, const TCHAR* Desc, std::initializer_list<FLevel> Values)
	{
		FArchetype A;
		A.Name = Name;
		A.Description = Desc;
		for (const FLevel& V : Values) { A.Skills.Add(FName(V.Id), V.Level); }
		return A;
	};
	Archetypes.Reset();
	Archetypes.Add(MakeArch(TEXT("Le Zélé"), TEXT("Patriote sans états d'âme, la poigne facile. La voix de la Révolution parle fort dans sa tête."),
		{ {TEXT("ferveur"),4}, {TEXT("poigne"),4}, {TEXT("endurance"),3}, {TEXT("volonte"),3}, {TEXT("sangfroid"),2},
		  {TEXT("perception"),2}, {TEXT("logique"),1}, {TEXT("rhetorique"),1}, {TEXT("empathie"),1}, {TEXT("spectres"),1} }));
	Archetypes.Add(MakeArch(TEXT("L'Avocat"), TEXT("Formé au barreau avant 89. Observe, déduit, ment avec aplomb. Le corps suit mal."),
		{ {TEXT("logique"),4}, {TEXT("rhetorique"),4}, {TEXT("perception"),3}, {TEXT("sangfroid"),3}, {TEXT("ferveur"),2},
		  {TEXT("volonte"),2}, {TEXT("empathie"),2}, {TEXT("spectres"),1}, {TEXT("endurance"),1}, {TEXT("poigne"),1} }));
	Archetypes.Add(MakeArch(TEXT("Le Hanté"), TEXT("Sent tout, voit les morts de la place. Fragile."),
		{ {TEXT("empathie"),4}, {TEXT("spectres"),4}, {TEXT("volonte"),3}, {TEXT("perception"),2}, {TEXT("rhetorique"),2},
		  {TEXT("logique"),2}, {TEXT("endurance"),2}, {TEXT("sangfroid"),1}, {TEXT("poigne"),1}, {TEXT("ferveur"),1} }));

	using FSpeaker = TPair<FString, EDialogueLineKind>;
	Speakers.Add(TEXT("marie"), FSpeaker(TEXT("Marie-Anne Delorme"), EDialogueLineKind::Character));
	Speakers.Add(TEXT("messager"), FSpeaker(TEXT("Estafette du Comité"), EDialogueLineKind::Official));
	Speakers.Add(TEXT("crieur"), FSpeaker(TEXT("Crieur public"), EDialogueLineKind::Official));
	Speakers.Add(TEXT("porteur"), FSpeaker(TEXT("Le porteur d'eau"), EDialogueLineKind::Character));
	Speakers.Add(TEXT("mercier"), FSpeaker(TEXT("Brutus Mercier"), EDialogueLineKind::Character));

	MusicDescriptions.Add(TEXT("Musique.Reveil"),  TEXT("violoncelle seul et nappe grave"));
	MusicDescriptions.Add(TEXT("Musique.Place"),   TEXT("tambour lent, cordes frottées, « Ça ira » déformé"));
	MusicDescriptions.Add(TEXT("Musique.Tension"), TEXT("percussions sèches, cordes en trémolo"));
	MusicDescriptions.Add(TEXT("Musique.Fin"),     TEXT("accord suspendu"));

	LoadScenes();
}

void UDialogueSubsystem::LoadScenes()
{
	Scenes.Reset();
	const FString Dir = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Dialogues"));
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(Dir, TEXT("*.txt")), true, false);
	Files.Sort();

	int32 Order = 0;
	for (const FString& File : Files)
	{
		FString Text;
		const FString Path = FPaths::Combine(Dir, File);
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			UE_LOG(LogThermidor, Error, TEXT("Impossible de lire %s"), *Path);
			continue;
		}
		TArray<FKVPair> Root;
		FString Error;
		if (!KeyValues::Parse(Text, Root, Error))
		{
			UE_LOG(LogThermidor, Error, TEXT("%s : %s"), *File, *Error);
			continue;
		}
		const FKVPair* Dialogue = nullptr;
		for (const FKVPair& P : Root)
		{
			if (P.bIsBlock && P.Key == TEXT("dialogue")) { Dialogue = &P; break; }
		}
		if (!Dialogue)
		{
			UE_LOG(LogThermidor, Error, TEXT("%s : bloc \"dialogue\" introuvable"), *File);
			continue;
		}
		FDialogueScene Scene;
		Scene.Id = FName(*Dialogue->Get(TEXT("id")));
		Scene.Title = Dialogue->Get(TEXT("title"));
		Scene.Start = Dialogue->Get(TEXT("start"));
		const FString NextStr = Dialogue->Get(TEXT("next"));
		Scene.Next = NextStr.IsEmpty() ? NAME_None : FName(*NextStr);
		Scene.Order = Order++;
		if (const FKVPair* Nodes = Dialogue->GetBlock(TEXT("nodes")))
		{
			for (const FKVPair& Node : Nodes->Children)
			{
				if (Node.bIsBlock) { Scene.Nodes.Add(Node.Key, Node); }
			}
		}
		UE_LOG(LogThermidor, Log, TEXT("Scène %s chargée : %d noeuds"), *Scene.Id.ToString(), Scene.Nodes.Num());
		Scenes.Add(Scene.Id, MoveTemp(Scene));
	}
}

FName UDialogueSubsystem::GetFirstSceneId() const
{
	FName Best = NAME_None;
	int32 BestOrder = MAX_int32;
	for (const TPair<FName, FDialogueScene>& It : Scenes)
	{
		if (It.Value.Order < BestOrder) { BestOrder = It.Value.Order; Best = It.Key; }
	}
	return Best;
}

void UDialogueSubsystem::NewGame(int32 ArchetypeIndex)
{
	if (!Archetypes.IsValidIndex(ArchetypeIndex)) { return; }
	const FArchetype& A = Archetypes[ArchetypeIndex];
	ArchetypeName = A.Name;
	Skills = A.Skills;
	Flags.Reset();
	Tried.Reset();
	FailedWhite.Reset();
	MaxHealth = GetSkill(TEXT("endurance")) + 2;
	MaxMorale = GetSkill(TEXT("volonte")) + 2;
	Health = MaxHealth;
	Morale = MaxMorale;
	Xp = 0;
	Points = 0;
	bGameOver = false;
	bInGame = true;
	StartScene(GetFirstSceneId());
}

void UDialogueSubsystem::ReturnToTitle()
{
	bInGame = false;
	bGameOver = false;
	bSceneEnded = false;
	Log.Reset();
	ChoiceViews.Reset();
	VisibleChoiceIndices.Reset();
	OnChanged.Broadcast();
}

bool UDialogueSubsystem::StartScene(FName SceneId)
{
	const FDialogueScene* Scene = Scenes.Find(SceneId);
	if (!Scene)
	{
		UE_LOG(LogThermidor, Error, TEXT("Scène introuvable : %s"), *SceneId.ToString());
		return false;
	}
	CurrentScene = SceneId;
	NextScene = Scene->Next;
	bSceneEnded = false;
	Log.Reset();
	ChoiceViews.Reset();
	VisibleChoiceIndices.Reset();
	OnSceneStarted.Broadcast(SceneId);
	Enter(Scene->Start);
	return true;
}

FString UDialogueSubsystem::GetCurrentSceneTitle() const
{
	const FDialogueScene* Scene = Scenes.Find(CurrentScene);
	return Scene ? Scene->Title : FString();
}

FString UDialogueSubsystem::DescribeMusic(FName MusicId) const
{
	const FString* D = MusicDescriptions.Find(MusicId);
	return D ? *D : FString();
}

const FKVPair* UDialogueSubsystem::GetCurrentNode() const
{
	const FDialogueScene* Scene = Scenes.Find(CurrentScene);
	return Scene ? Scene->Nodes.Find(CurrentNode) : nullptr;
}

const FSkillInfo* UDialogueSubsystem::FindSkill(FName Id) const
{
	for (const FSkillInfo& S : GetSkillInfos())
	{
		if (S.Id == Id) { return &S; }
	}
	return nullptr;
}

bool UDialogueSubsystem::IsAtHub() const
{
	const FKVPair* Node = GetCurrentNode();
	return Node && Node->Has(TEXT("hub")) && !bSceneEnded && !bGameOver;
}

bool UDialogueSubsystem::Passes(const FKVPair& Pair) const
{
	for (const FString& F : Pair.GetAll(TEXT("require")))
	{
		if (!Flags.Contains(F)) { return false; }
	}
	for (const FString& F : Pair.GetAll(TEXT("unless")))
	{
		if (Flags.Contains(F)) { return false; }
	}
	return true;
}

FString UDialogueSubsystem::ChoiceKey(int32 ChoiceIndex) const
{
	return FString::Printf(TEXT("%s:%s:%d"), *CurrentScene.ToString(), *CurrentNode, ChoiceIndex);
}

int32 UDialogueSubsystem::ChanceOfSuccess(int32 Level, int32 Difficulty)
{
	int32 Ok = 0;
	for (int32 A = 1; A <= 6; ++A)
	{
		for (int32 B = 1; B <= 6; ++B)
		{
			if (A == 6 && B == 6) { ++Ok; }
			else if (A == 1 && B == 1) {}
			else if (A + B + Level >= Difficulty) { ++Ok; }
		}
	}
	return FMath::RoundToInt(Ok * 100.f / 36.f);
}

FString UDialogueSubsystem::DifficultyName(int32 D)
{
	if (D <= 6) return TEXT("Trivial");
	if (D <= 8) return TEXT("Facile");
	if (D <= 10) return TEXT("Moyen");
	if (D <= 12) return TEXT("Difficile");
	if (D <= 14) return TEXT("Redoutable");
	return TEXT("Héroïque");
}

void UDialogueSubsystem::AddLine(const FDialogueLineView& View)
{
	Log.Add(View);
	OnLine.Broadcast(View);
}

void UDialogueSubsystem::Say(const FKVPair& Line)
{
	const FString Who = Line.Get(TEXT("speaker"));
	const FString Passive = Line.Get(TEXT("passive"));
	if (!Passive.IsEmpty())
	{
		const int32 Difficulty = FCString::Atoi(*Line.Get(TEXT("difficulty")));
		if (GetSkill(FName(*Passive)) + 6 < Difficulty) { return; }
	}

	FDialogueLineView V;
	V.Text = Line.Get(TEXT("text"));
	if (Who == TEXT("narrateur"))
	{
		V.Kind = EDialogueLineKind::Narration;
		V.Color = ColText;
	}
	else if (const FSkillInfo* Skill = FindSkill(FName(*Who)))
	{
		V.Kind = EDialogueLineKind::Skill;
		V.Speaker = Skill->Name.ToUpper();
		V.Color = Skill->Color;
		if (!Passive.IsEmpty())
		{
			V.Tag = FString::Printf(TEXT("[%s : réussite]"), *DifficultyName(FCString::Atoi(*Line.Get(TEXT("difficulty")))));
		}
	}
	else if (const TPair<FString, EDialogueLineKind>* Sp = Speakers.Find(Who))
	{
		V.Kind = Sp->Value;
		V.Speaker = Sp->Key.ToUpper();
		V.Color = Sp->Value == EDialogueLineKind::Official ? ColOfficial : ColText;
	}
	else
	{
		V.Kind = EDialogueLineKind::Character;
		V.Speaker = Who.ToUpper();
		V.Color = ColText;
	}
	AddLine(V);
}

void UDialogueSubsystem::ChangeGauge(bool bHealth, int32 Delta)
{
	int32& Value = bHealth ? Health : Morale;
	const int32 Max = bHealth ? MaxHealth : MaxMorale;
	Value = FMath::Clamp(Value + Delta, 0, Max);

	FDialogueLineView V;
	V.Kind = EDialogueLineKind::Gauge;
	V.Text = FString::Printf(TEXT("%s %s%d"), bHealth ? TEXT("Santé") : TEXT("Moral"), Delta > 0 ? TEXT("+") : TEXT(""), Delta);
	V.Color = Delta > 0 ? ColGood : ColBad;
	AddLine(V);

	if (Value <= 0)
	{
		bGameOver = true;
		FDialogueLineView End;
		End.Kind = EDialogueLineKind::Narration;
		End.Color = ColText;
		End.Text = bHealth
			? TEXT("Ton corps lâche. Le pavé revient contre ta joue, et cette fois il y reste.")
			: TEXT("Quelque chose en toi s'éteint. Tu noues l'écharpe sans réfléchir, et tu n'entends plus rien.");
		AddLine(End);
		ChoiceViews.Reset();
		VisibleChoiceIndices.Reset();
		OnSceneEnded.Broadcast();
	}
	OnChanged.Broadcast();
}

void UDialogueSubsystem::GainXp(int32 Amount)
{
	Xp += Amount;
	while (Xp >= XpPerPoint)
	{
		Xp -= XpPerPoint;
		++Points;
		FDialogueLineView V;
		V.Kind = EDialogueLineKind::Gauge;
		V.Color = ColPlayer;
		V.Text = TEXT("+1 point de compétence. Dépense-le dans le dossier.");
		AddLine(V);
	}
	OnChanged.Broadcast();
}

void UDialogueSubsystem::Enter(const FString& NodeId)
{
	const FDialogueScene* Scene = Scenes.Find(CurrentScene);
	const FKVPair* Node = Scene ? Scene->Nodes.Find(NodeId) : nullptr;
	if (!Node)
	{
		FDialogueLineView V;
		V.Kind = EDialogueLineKind::System;
		V.Color = ColBad;
		V.Text = FString::Printf(TEXT("Noeud introuvable : %s"), *NodeId);
		AddLine(V);
		UE_LOG(LogThermidor, Error, TEXT("Noeud introuvable : %s/%s"), *CurrentScene.ToString(), *NodeId);
		return;
	}
	CurrentNode = NodeId;

	for (const FKVPair& Entry : Node->Children)
	{
		if (bGameOver) { return; }
		if (Entry.Key == TEXT("music"))
		{
			CurrentMusic = FName(*Entry.Value);
			OnMusic.Broadcast(CurrentMusic);
		}
		else if (Entry.Key == TEXT("set"))
		{
			Flags.Add(Entry.Value);
		}
		else if (Entry.Key == TEXT("health") || Entry.Key == TEXT("morale"))
		{
			ChangeGauge(Entry.Key == TEXT("health"), FCString::Atoi(*Entry.Value));
		}
		else if (Entry.Key == TEXT("line") && Entry.bIsBlock && Passes(Entry))
		{
			Say(Entry);
		}
	}
	if (bGameOver) { return; }

	if (Node->Has(TEXT("end")))
	{
		const FString NodeNext = Node->Get(TEXT("next"));
		if (!NodeNext.IsEmpty()) { NextScene = FName(*NodeNext); }
		bSceneEnded = true;
		ChoiceViews.Reset();
		VisibleChoiceIndices.Reset();
		GainXp(2);
		OnSceneEnded.Broadcast();
		OnChanged.Broadcast();
		return;
	}
	RebuildChoices();
	OnChanged.Broadcast();
}

void UDialogueSubsystem::RebuildChoices()
{
	ChoiceViews.Reset();
	VisibleChoiceIndices.Reset();
	const FKVPair* Node = GetCurrentNode();
	if (!Node || bGameOver || bSceneEnded) { return; }

	const TArray<const FKVPair*> Choices = Node->GetAllBlocks(TEXT("choice"));
	for (int32 i = 0; i < Choices.Num(); ++i)
	{
		const FKVPair& C = *Choices[i];
		if (!Passes(C)) { continue; }
		const FString Key = ChoiceKey(i);
		const FString Check = C.Get(TEXT("check"));
		const FString Type = C.Get(TEXT("type"));
		if (!Check.IsEmpty() && Type == TEXT("rouge") && Tried.Contains(Key)) { continue; }

		FDialogueChoiceView V;
		V.Number = ChoiceViews.Num() + 1;
		V.Text = C.Get(TEXT("text"));
		if (!Check.IsEmpty())
		{
			const FSkillInfo* Skill = FindSkill(FName(*Check));
			const FString SkillName = Skill ? Skill->Name : Check;
			const int32 D = FCString::Atoi(*C.Get(TEXT("difficulty")));
			const int32 Level = GetSkill(FName(*Check));
			V.bRedCheck = Type == TEXT("rouge");
			V.bWhiteCheck = !V.bRedCheck;
			const int32* FailedAt = FailedWhite.Find(Key);
			if (FailedAt && Level <= *FailedAt)
			{
				V.bDisabled = true;
				V.CheckLabel = FString::Printf(TEXT("[%s · raté. Monte la compétence pour retenter]"), *SkillName);
			}
			else
			{
				V.CheckLabel = FString::Printf(TEXT("[%s · %s %d · %d %%]"), *SkillName, *DifficultyName(D), D, ChanceOfSuccess(Level, D));
			}
		}
		ChoiceViews.Add(V);
		VisibleChoiceIndices.Add(i);
	}
}

void UDialogueSubsystem::ChooseNumber(int32 Number)
{
	const int32 Visible = Number - 1;
	if (!ChoiceViews.IsValidIndex(Visible) || ChoiceViews[Visible].bDisabled) { return; }
	ChooseInternal(VisibleChoiceIndices[Visible]);
}

static FString ChoiceTarget(const FKVPair& C)
{
	const FString Goto = C.Get(TEXT("goto"));
	return Goto.IsEmpty() ? C.Get(TEXT("success")) : Goto;
}

bool UDialogueSubsystem::HasChoiceForTarget(const FString& TargetNode) const
{
	const FKVPair* Node = GetCurrentNode();
	if (!Node) { return false; }
	const TArray<const FKVPair*> Choices = Node->GetAllBlocks(TEXT("choice"));
	for (int32 v = 0; v < VisibleChoiceIndices.Num(); ++v)
	{
		if (!ChoiceViews[v].bDisabled && ChoiceTarget(*Choices[VisibleChoiceIndices[v]]) == TargetNode) { return true; }
	}
	return false;
}

bool UDialogueSubsystem::ChooseByTarget(const FString& TargetNode)
{
	const FKVPair* Node = GetCurrentNode();
	if (!Node) { return false; }
	const TArray<const FKVPair*> Choices = Node->GetAllBlocks(TEXT("choice"));
	// Deux passes : d'abord les choix sans test (un indice trouvé vaut mieux qu'un jet de dés).
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (int32 v = 0; v < VisibleChoiceIndices.Num(); ++v)
		{
			const FKVPair& C = *Choices[VisibleChoiceIndices[v]];
			const bool bHasCheck = C.Has(TEXT("check"));
			if ((Pass == 0) == bHasCheck) { continue; }
			if (!ChoiceViews[v].bDisabled && ChoiceTarget(C) == TargetNode)
			{
				ChooseInternal(VisibleChoiceIndices[v]);
				return true;
			}
		}
	}
	return false;
}

void UDialogueSubsystem::ChooseInternal(int32 ChoiceIndex)
{
	const FKVPair* Node = GetCurrentNode();
	if (!Node) { return; }
	const TArray<const FKVPair*> Choices = Node->GetAllBlocks(TEXT("choice"));
	if (!Choices.IsValidIndex(ChoiceIndex)) { return; }
	// Copie : Enter() change de noeud et peut invalider la référence.
	const FKVPair C = *Choices[ChoiceIndex];
	const FString Key = ChoiceKey(ChoiceIndex);

	ChoiceViews.Reset();
	VisibleChoiceIndices.Reset();

	FDialogueLineView Mine;
	Mine.Kind = EDialogueLineKind::Player;
	Mine.Speaker = TEXT("TOI");
	Mine.Text = C.Get(TEXT("text"));
	Mine.Color = ColPlayer;
	AddLine(Mine);

	const FString Check = C.Get(TEXT("check"));
	if (Check.IsEmpty())
	{
		Enter(C.Get(TEXT("goto")));
		return;
	}

	const int32 D = FCString::Atoi(*C.Get(TEXT("difficulty")));
	const int32 Level = GetSkill(FName(*Check));
	const FString Type = C.Get(TEXT("type"));
	const int32 A = FMath::RandRange(1, 6);
	const int32 B = FMath::RandRange(1, 6);
	const bool bOk = (A == 6 && B == 6) || (!(A == 1 && B == 1) && A + B + Level >= D);
	Tried.Add(Key);
	if (Type == TEXT("blanc"))
	{
		if (bOk) { FailedWhite.Remove(Key); } else { FailedWhite.Add(Key, Level); }
	}

	const FSkillInfo* Skill = FindSkill(FName(*Check));
	FDialogueLineView Roll;
	Roll.Kind = EDialogueLineKind::Roll;
	Roll.Die1 = A;
	Roll.Die2 = B;
	Roll.bSuccess = bOk;
	Roll.Color = bOk ? ColGood : ColBad;
	const TCHAR* Why = (A == 6 && B == 6) ? TEXT(" (double six)") : (A == 1 && B == 1) ? TEXT(" (double un)") : TEXT("");
	Roll.Text = FString::Printf(TEXT("Test %s de %s : %d + %d + %d = %d contre %d%s → %s"),
		*Type, Skill ? *Skill->Name : *Check, A, B, Level, A + B + Level, D, Why, bOk ? TEXT("Réussite") : TEXT("Échec"));
	AddLine(Roll);

	if (bOk) { GainXp(1); }
	Enter(C.Get(bOk ? TEXT("success") : TEXT("failure")));
}

void UDialogueSubsystem::SpendPoint(FName SkillId)
{
	if (Points <= 0 || bGameOver || !Skills.Contains(SkillId)) { return; }
	--Points;
	Skills[SkillId] += 1;
	if (SkillId == TEXT("endurance")) { ++MaxHealth; ++Health; }
	if (SkillId == TEXT("volonte")) { ++MaxMorale; ++Morale; }
	RebuildChoices();
	OnChanged.Broadcast();
}

TArray<FString> UDialogueSubsystem::GetMeaningfulFlags() const
{
	TArray<FString> Out;
	for (const FString& F : Flags)
	{
		if (!F.StartsWith(TEXT("vu_"))) { Out.Add(F); }
	}
	return Out;
}
