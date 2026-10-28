#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ThermidorPlayerController.generated.h"

class SDialoguePanel;
class AInteractif;
class UDialogueSubsystem;

/**
 * Passe du carnet de dialogue (souris visible, déplacement bloqué)
 * à l'exploration (souris capturée, déplacement libre) et inversement.
 * On ne peut explorer que depuis un noeud marqué "hub".
 */
UCLASS()
class HUITTHERMIDOR_API AThermidorPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void PlayerTick(float DeltaTime) override;

	void ToggleExplore();
	void InteractWith(AInteractif* Target);
	void ChooseNumber(int32 Number);

	bool IsExploring() const { return bExploring; }
	FText GetHintText() const;

private:
	UDialogueSubsystem* GetDialogue() const;
	void SetExploring(bool bInExploring);
	void ApplyInputMode();
	void HandleDialogueLine();

	TSharedPtr<SDialoguePanel> Panel;
	TSharedPtr<SWidget> PanelContainer;
	bool bExploring = false;
	bool bInputApplied = false;
	bool bLastExploring = false;
	bool bLastInGame = false;
	FString TransientHint;
	float TransientHintTime = 0.f;
	FDelegateHandle LineHandle;
};
