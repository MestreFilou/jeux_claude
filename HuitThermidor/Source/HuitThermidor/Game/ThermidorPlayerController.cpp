#include "Game/ThermidorPlayerController.h"
#include "Game/Commissaire.h"
#include "Game/Interactif.h"
#include "Dialogue/DialogueSubsystem.h"
#include "UI/SDialoguePanel.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWeakWidget.h"

UDialogueSubsystem* AThermidorPlayerController::GetDialogue() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UDialogueSubsystem>() : nullptr;
}

void AThermidorPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) { return; }

	UDialogueSubsystem* Dialogue = GetDialogue();
	TWeakObjectPtr<AThermidorPlayerController> WeakThis(this);

	SAssignNew(Panel, SDialoguePanel)
		.Subsystem(Dialogue)
		.IsExploring_Lambda([WeakThis]() { return WeakThis.IsValid() && WeakThis->IsExploring(); })
		.HintText_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->GetHintText() : FText::GetEmpty(); });

	PanelContainer = SNew(SWeakWidget).PossiblyNullContent(Panel);
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->AddViewportWidgetContent(PanelContainer.ToSharedRef(), 10);
	}

	if (Dialogue)
	{
		// Une nouvelle ligne pendant l'exploration (objet examiné...) rouvre le carnet.
		LineHandle = Dialogue->OnLine.AddWeakLambda(this, [this](const FDialogueLineView&) { HandleDialogueLine(); });
	}
	ApplyInputMode();
}

void AThermidorPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UDialogueSubsystem* Dialogue = GetDialogue())
	{
		Dialogue->OnLine.Remove(LineHandle);
	}
	if (PanelContainer.IsValid())
	{
		if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
		{
			Viewport->RemoveViewportWidgetContent(PanelContainer.ToSharedRef());
		}
	}
	Panel.Reset();
	PanelContainer.Reset();
	Super::EndPlay(Reason);
}

void AThermidorPlayerController::HandleDialogueLine()
{
	if (bExploring) { SetExploring(false); }
}

void AThermidorPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (TransientHintTime > 0.f)
	{
		TransientHintTime -= DeltaTime;
		if (TransientHintTime <= 0.f) { TransientHint.Reset(); }
	}

	// Hors d'un hub, le carnet reste ouvert : la conversation est en cours.
	const UDialogueSubsystem* Dialogue = GetDialogue();
	const bool bInGame = Dialogue && Dialogue->IsInGame();
	if (bExploring && (!bInGame || !Dialogue->IsAtHub()))
	{
		bExploring = false;
	}
	if (!bInputApplied || bExploring != bLastExploring || bInGame != bLastInGame)
	{
		bLastExploring = bExploring;
		bLastInGame = bInGame;
		ApplyInputMode();
	}
}

void AThermidorPlayerController::SetExploring(bool bInExploring)
{
	const UDialogueSubsystem* Dialogue = GetDialogue();
	if (bInExploring && (!Dialogue || !Dialogue->IsAtHub()))
	{
		return;
	}
	bExploring = bInExploring;
	ApplyInputMode();
	bLastExploring = bExploring;
}

void AThermidorPlayerController::ApplyInputMode()
{
	bInputApplied = true;
	ResetIgnoreInputFlags();
	if (bExploring)
	{
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
	else
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		SetInputMode(Mode);
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
	}
}

void AThermidorPlayerController::ToggleExplore()
{
	SetExploring(!bExploring);
	if (!bExploring && TransientHint.IsEmpty())
	{
		const UDialogueSubsystem* Dialogue = GetDialogue();
		if (Dialogue && Dialogue->IsInGame() && !Dialogue->IsAtHub() && !Dialogue->IsSceneEnded())
		{
			TransientHint = TEXT("Termine d'abord la conversation en cours.");
			TransientHintTime = 2.5f;
		}
	}
}

void AThermidorPlayerController::InteractWith(AInteractif* Target)
{
	if (!bExploring || !Target) { return; }
	UDialogueSubsystem* Dialogue = GetDialogue();
	if (!Dialogue) { return; }
	for (const FString& Node : Target->TargetNodes)
	{
		if (Dialogue->ChooseByTarget(Node))
		{
			SetExploring(false);
			return;
		}
	}
	TransientHint = Target->IdleText;
	TransientHintTime = 2.5f;
}

void AThermidorPlayerController::ChooseNumber(int32 Number)
{
	if (bExploring) { return; }
	if (UDialogueSubsystem* Dialogue = GetDialogue())
	{
		if (Dialogue->IsInGame()) { Dialogue->ChooseNumber(Number); }
	}
}

FText AThermidorPlayerController::GetHintText() const
{
	if (!TransientHint.IsEmpty())
	{
		return FText::FromString(TransientHint);
	}
	if (!bExploring) { return FText::GetEmpty(); }

	const ACommissaire* Pawn = Cast<ACommissaire>(GetPawn());
	const AInteractif* Target = Pawn ? Pawn->GetFocused() : nullptr;
	const UDialogueSubsystem* Dialogue = GetDialogue();
	if (Target && Dialogue)
	{
		bool bAvailable = false;
		for (const FString& Node : Target->TargetNodes)
		{
			if (Dialogue->HasChoiceForTarget(Node)) { bAvailable = true; break; }
		}
		return FText::FromString(bAvailable
			? FString::Printf(TEXT("E : %s %s"), *Target->Verb, *Target->DisplayName)
			: Target->DisplayName);
	}
	return FText::FromString(TEXT("Tab : rouvrir le carnet"));
}
