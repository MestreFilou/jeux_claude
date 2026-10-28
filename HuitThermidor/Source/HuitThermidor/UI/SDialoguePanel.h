#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Dialogue/DialogueTypes.h"

class UDialogueSubsystem;
class SScrollBox;
class SVerticalBox;

/**
 * L'interface façon Disco Elysium, écrite en Slate (aucun fichier .uasset) :
 * - à gauche, le dossier du commissaire (jauges, compétences, expérience, musique) ;
 * - à droite, la colonne de dialogue qui défile, puis les choix numérotés ;
 * - au centre, l'écran de départ (choix du profil).
 */
class HUITTHERMIDOR_API SDialoguePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialoguePanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UDialogueSubsystem>, Subsystem)
		/** Vrai quand le joueur se promène dans le décor (colonne masquée). */
		SLATE_ATTRIBUTE(bool, IsExploring)
		/** Texte d'aide en bas de l'écran (cible visée, touches). */
		SLATE_ATTRIBUTE(FText, HintText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SDialoguePanel() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void HandleLine(const FDialogueLineView& Line);
	void HandleChanged();
	void HandleSceneStarted(FName SceneId);

	void RebuildLog();
	void RebuildChoices();
	void RebuildDossier();
	void RebuildStart();
	TSharedRef<SWidget> MakeLineWidget(const FDialogueLineView& Line) const;

	EVisibility GetColumnVisibility() const;
	EVisibility GetDossierVisibility() const;
	EVisibility GetStartVisibility() const;
	EVisibility GetHintVisibility() const;

	TWeakObjectPtr<UDialogueSubsystem> Subsystem;
	TAttribute<bool> IsExploring;
	TAttribute<FText> HintText;

	TSharedPtr<SScrollBox> LogBox;
	TSharedPtr<SVerticalBox> ChoicesBox;
	TSharedPtr<SVerticalBox> DossierBox;
	TSharedPtr<SVerticalBox> StartBox;
	bool bScrollPending = false;
};
