#include "UI/SDialoguePanel.h"
#include "Dialogue/DialogueSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Thermidor"

namespace
{
	const FLinearColor Ink(0.89f, 0.85f, 0.78f);
	const FLinearColor Muted(0.58f, 0.53f, 0.47f);
	const FLinearColor Gold(0.91f, 0.76f, 0.48f);
	const FLinearColor ChoiceCol(0.79f, 0.65f, 0.36f);
	const FLinearColor Disabled(0.36f, 0.38f, 0.37f);
	const FLinearColor Red(0.88f, 0.45f, 0.42f);
	const FLinearColor PanelBg(0.063f, 0.051f, 0.047f, 0.93f);
	const FLinearColor DossierBg(0.063f, 0.051f, 0.047f, 0.80f);

	FSlateFontInfo Font(const char* Style, int32 Size) { return FCoreStyle::GetDefaultFontStyle(Style, Size); }
	const FSlateBrush* White() { return FCoreStyle::Get().GetBrush("WhiteBrush"); }

	FString Pips(int32 Value, int32 Max)
	{
		FString S;
		for (int32 i = 0; i < Max; ++i) { S += (i < Value) ? TEXT("■ ") : TEXT("□ "); }
		return S;
	}
}

void SDialoguePanel::Construct(const FArguments& InArgs)
{
	Subsystem = InArgs._Subsystem;
	IsExploring = InArgs._IsExploring;
	HintText = InArgs._HintText;

	ChildSlot
	[
		SNew(SOverlay)

		// Dossier (gauche) et colonne de dialogue (droite)
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(24.f).VAlign(VAlign_Top)
			[
				SNew(SBox).WidthOverride(290.f).Visibility(this, &SDialoguePanel::GetDossierVisibility)
				[
					SNew(SBorder).BorderImage(White()).BorderBackgroundColor(DossierBg).Padding(16.f)
					[
						SAssignNew(DossierBox, SVerticalBox)
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SSpacer)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(620.f).Visibility(this, &SDialoguePanel::GetColumnVisibility)
				[
					SNew(SBorder).BorderImage(White()).BorderBackgroundColor(PanelBg).Padding(FMargin(28.f, 24.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
						[
							SNew(STextBlock)
							.Font(Font("Regular", 11))
							.ColorAndOpacity(FSlateColor(Muted))
							.AutoWrapText(true)
							.Text_Lambda([this]()
							{
								const UDialogueSubsystem* S = Subsystem.Get();
								return S ? FText::FromString(S->GetCurrentSceneTitle().ToUpper()) : FText::GetEmpty();
							})
						]
						+ SVerticalBox::Slot().FillHeight(1.f)
						[
							SAssignNew(LogBox, SScrollBox)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
						[
							SAssignNew(ChoicesBox, SVerticalBox)
						]
					]
				]
			]
		]

		// Écran de départ
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(760.f).Visibility(this, &SDialoguePanel::GetStartVisibility)
			[
				SNew(SBorder).BorderImage(White()).BorderBackgroundColor(PanelBg).Padding(32.f)
				[
					SAssignNew(StartBox, SVerticalBox)
				]
			]
		]

		// Aide en bas de l'écran
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 40.f)
		[
			SNew(SBorder).BorderImage(White()).BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.55f)).Padding(FMargin(14.f, 8.f))
			.Visibility(this, &SDialoguePanel::GetHintVisibility)
			[
				SNew(STextBlock).Font(Font("Regular", 13)).ColorAndOpacity(FSlateColor(Ink)).Text(HintText)
			]
		]

		// Viseur au centre quand on explore
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(Font("Regular", 14)).ColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.6f)))
			.Text(FText::FromString(TEXT("·")))
			.Visibility_Lambda([this]() { return IsExploring.Get(false) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		]
	];

	if (UDialogueSubsystem* S = Subsystem.Get())
	{
		S->OnLine.AddSP(this, &SDialoguePanel::HandleLine);
		S->OnChanged.AddSP(this, &SDialoguePanel::HandleChanged);
		S->OnSceneStarted.AddSP(this, &SDialoguePanel::HandleSceneStarted);
	}
	RebuildStart();
	RebuildLog();
	RebuildChoices();
	RebuildDossier();
}

SDialoguePanel::~SDialoguePanel()
{
	if (UDialogueSubsystem* S = Subsystem.Get())
	{
		S->OnLine.RemoveAll(this);
		S->OnChanged.RemoveAll(this);
		S->OnSceneStarted.RemoveAll(this);
	}
}

void SDialoguePanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (bScrollPending && LogBox.IsValid())
	{
		LogBox->ScrollToEnd();
		bScrollPending = false;
	}
}

EVisibility SDialoguePanel::GetColumnVisibility() const
{
	const UDialogueSubsystem* S = Subsystem.Get();
	if (!S || !S->IsInGame() || IsExploring.Get(false)) { return EVisibility::Collapsed; }
	return EVisibility::Visible;
}

EVisibility SDialoguePanel::GetDossierVisibility() const
{
	return GetColumnVisibility();
}

EVisibility SDialoguePanel::GetStartVisibility() const
{
	const UDialogueSubsystem* S = Subsystem.Get();
	return (S && !S->IsInGame()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SDialoguePanel::GetHintVisibility() const
{
	const UDialogueSubsystem* S = Subsystem.Get();
	if (!S || !S->IsInGame()) { return EVisibility::Collapsed; }
	return HintText.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
}

void SDialoguePanel::HandleLine(const FDialogueLineView& Line)
{
	if (LogBox.IsValid())
	{
		LogBox->AddSlot().Padding(0.f, 0.f, 0.f, 10.f)[ MakeLineWidget(Line) ];
		bScrollPending = true;
	}
}

void SDialoguePanel::HandleChanged()
{
	RebuildChoices();
	RebuildDossier();
}

void SDialoguePanel::HandleSceneStarted(FName SceneId)
{
	RebuildLog();
	RebuildDossier();
}

TSharedRef<SWidget> SDialoguePanel::MakeLineWidget(const FDialogueLineView& Line) const
{
	FString Display;
	FSlateFontInfo LineFont = Font("Regular", 15);
	switch (Line.Kind)
	{
	case EDialogueLineKind::Narration:
		Display = Line.Text;
		LineFont = Font("Italic", 15);
		break;
	case EDialogueLineKind::Roll:
	case EDialogueLineKind::Gauge:
	case EDialogueLineKind::System:
		Display = Line.Text;
		LineFont = Font("Regular", 12);
		break;
	default:
		Display = Line.Speaker;
		if (!Line.Tag.IsEmpty()) { Display += TEXT("  ") + Line.Tag; }
		Display += TEXT("  —  ") + Line.Text;
		break;
	}
	return SNew(STextBlock)
		.Text(FText::FromString(Display))
		.Font(LineFont)
		.ColorAndOpacity(FSlateColor(Line.Color))
		.AutoWrapText(true);
}

void SDialoguePanel::RebuildLog()
{
	if (!LogBox.IsValid()) { return; }
	LogBox->ClearChildren();
	if (const UDialogueSubsystem* S = Subsystem.Get())
	{
		for (const FDialogueLineView& Line : S->GetLog())
		{
			LogBox->AddSlot().Padding(0.f, 0.f, 0.f, 10.f)[ MakeLineWidget(Line) ];
		}
	}
	bScrollPending = true;
}

void SDialoguePanel::RebuildChoices()
{
	if (!ChoicesBox.IsValid()) { return; }
	ChoicesBox->ClearChildren();
	UDialogueSubsystem* S = Subsystem.Get();
	if (!S || !S->IsInGame()) { return; }
	TWeakObjectPtr<UDialogueSubsystem> Weak = Subsystem;

	// Fin de partie ou fin de scène
	if (S->IsGameOver() || S->IsSceneEnded())
	{
		const bool bOver = S->IsGameOver();
		const FName Next = S->GetNextSceneId();
		ChoicesBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(STextBlock).Font(Font("Bold", 14)).ColorAndOpacity(FSlateColor(Ink))
			.Text(bOver ? LOCTEXT("GameOver", "FIN DE PARTIE") : LOCTEXT("SceneEnd", "FIN DE LA SCÈNE"))
		];
		ChoicesBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
		[
			SNew(STextBlock).Font(Font("Regular", 11)).ColorAndOpacity(FSlateColor(Muted)).AutoWrapText(true)
			.Text(FText::FromString(TEXT("Ce que le jeu retient : ") + FString::Join(S->GetMeaningfulFlags(), TEXT(", "))))
		];
		if (!bOver && !Next.IsNone())
		{
			ChoicesBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([Weak, Next]() { if (Weak.IsValid()) { Weak->StartScene(Next); } return FReply::Handled(); })
				[
					SNew(STextBlock).Font(Font("Regular", 14)).Text(LOCTEXT("NextScene", "Scène suivante"))
				]
			];
		}
		if (bOver || Next.IsNone())
		{
			ChoicesBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SButton)
				.OnClicked_Lambda([Weak]() { if (Weak.IsValid()) { Weak->ReturnToTitle(); } return FReply::Handled(); })
				[
					SNew(STextBlock).Font(Font("Regular", 14)).Text(LOCTEXT("Replay", "Rejouer avec un autre commissaire"))
				]
			];
		}
		if (!bOver && Next.IsNone())
		{
			ChoicesBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock).Font(Font("Italic", 13)).ColorAndOpacity(FSlateColor(Muted))
				.Text(LOCTEXT("ToBeContinued", "La suite n'est pas encore écrite."))
			];
		}
		return;
	}

	for (const FDialogueChoiceView& C : S->GetChoices())
	{
		const int32 Number = C.Number;
		const FLinearColor TextCol = C.bDisabled ? Disabled : ChoiceCol;
		const FLinearColor CheckCol = C.bDisabled ? Disabled : (C.bRedCheck ? Red : Ink);

		TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);
		if (!C.CheckLabel.IsEmpty())
		{
			Content->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Font(Font("Bold", 12)).ColorAndOpacity(FSlateColor(CheckCol)).AutoWrapText(true)
				.Text(FText::FromString(C.CheckLabel))
			];
		}
		Content->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Font(Font("Regular", 15)).ColorAndOpacity(FSlateColor(TextCol)).AutoWrapText(true)
			.Text(FText::FromString(FString::Printf(TEXT("%d.  %s"), Number, *C.Text)))
		];

		ChoicesBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SButton)
			.ButtonStyle(&FCoreStyle::Get(), "NoBorder")
			.IsEnabled(!C.bDisabled)
			.OnClicked_Lambda([Weak, Number]() { if (Weak.IsValid()) { Weak->ChooseNumber(Number); } return FReply::Handled(); })
			[
				Content
			]
		];
	}

	if (S->IsAtHub())
	{
		ChoicesBox->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
		[
			SNew(STextBlock).Font(Font("Italic", 12)).ColorAndOpacity(FSlateColor(Muted))
			.Text(LOCTEXT("HubHint", "Tab : fermer le carnet et explorer les lieux (E pour interagir)."))
		];
	}
}

void SDialoguePanel::RebuildDossier()
{
	if (!DossierBox.IsValid()) { return; }
	DossierBox->ClearChildren();
	UDialogueSubsystem* S = Subsystem.Get();
	if (!S || !S->IsInGame()) { return; }
	TWeakObjectPtr<UDialogueSubsystem> Weak = Subsystem;

	DossierBox->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Font(Font("Regular", 10)).ColorAndOpacity(FSlateColor(Muted)).AutoWrapText(true)
		.Text(LOCTEXT("Org", "COMITÉ DE SURVEILLANCE, SECTION DES PIQUES"))
	];
	DossierBox->AddSlot().AutoHeight().Padding(0.f, 2.f, 0.f, 12.f)
	[
		SNew(STextBlock).Font(Font("Bold", 20)).ColorAndOpacity(FSlateColor(Ink))
		.Text(FText::FromString(S->GetArchetypeName().ToUpper()))
	];
	DossierBox->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Font(Font("Regular", 13)).ColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.45f, 0.42f)))
		.Text(FText::FromString(TEXT("Santé   ") + Pips(S->GetHealth(), S->GetMaxHealth())))
	];
	DossierBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
	[
		SNew(STextBlock).Font(Font("Regular", 13)).ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.55f, 0.88f)))
		.Text(FText::FromString(TEXT("Moral   ") + Pips(S->GetMorale(), S->GetMaxMorale())))
	];

	const bool bCanSpend = S->GetPoints() > 0 && !S->IsGameOver();
	FString LastAttribute;
	for (const FSkillInfo& Info : UDialogueSubsystem::GetSkillInfos())
	{
		if (Info.Attribute != LastAttribute)
		{
			LastAttribute = Info.Attribute;
			DossierBox->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 2.f)
			[
				SNew(STextBlock).Font(Font("Bold", 11)).ColorAndOpacity(FSlateColor(Info.Color))
				.Text(FText::FromString(Info.Attribute.ToUpper()))
			];
		}
		const FName Id = Info.Id;
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock).Font(Font("Regular", 13)).ColorAndOpacity(FSlateColor(Ink)).Text(FText::FromString(Info.Name))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock).Font(Font("Regular", 13)).ColorAndOpacity(FSlateColor(Muted))
				.Text(FText::AsNumber(S->GetSkill(Id)))
			];
		if (bCanSpend)
		{
			Row->AddSlot().AutoWidth().Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.ContentPadding(FMargin(6.f, 0.f))
				.OnClicked_Lambda([Weak, Id]() { if (Weak.IsValid()) { Weak->SpendPoint(Id); } return FReply::Handled(); })
				[
					SNew(STextBlock).Font(Font("Bold", 12)).ColorAndOpacity(FSlateColor(Gold)).Text(FText::FromString(TEXT("+")))
				]
			];
		}
		DossierBox->AddSlot().AutoHeight()[ Row ];
	}

	FString XpText = FString::Printf(TEXT("Expérience %d / %d"), S->GetXp(), S->GetXpPerPoint());
	if (S->GetPoints() > 0)
	{
		XpText += FString::Printf(TEXT("  ·  %d point%s à dépenser"), S->GetPoints(), S->GetPoints() > 1 ? TEXT("s") : TEXT(""));
	}
	DossierBox->AddSlot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
	[
		SNew(STextBlock).Font(Font("Regular", 11)).ColorAndOpacity(FSlateColor(S->GetPoints() > 0 ? Gold : Muted)).Text(FText::FromString(XpText))
	];

	const FName Music = S->GetCurrentMusic();
	if (!Music.IsNone())
	{
		DossierBox->AddSlot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
		[
			SNew(STextBlock).Font(Font("Regular", 10)).ColorAndOpacity(FSlateColor(Muted)).AutoWrapText(true)
			.Text(FText::FromString(FString::Printf(TEXT("♪ %s : %s"), *Music.ToString(), *S->DescribeMusic(Music))))
		];
	}
}

void SDialoguePanel::RebuildStart()
{
	if (!StartBox.IsValid()) { return; }
	StartBox->ClearChildren();
	UDialogueSubsystem* S = Subsystem.Get();
	if (!S) { return; }
	TWeakObjectPtr<UDialogueSubsystem> Weak = Subsystem;

	StartBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
	[
		SNew(STextBlock).Font(Font("Bold", 30)).ColorAndOpacity(FSlateColor(Ink)).Text(LOCTEXT("Title", "HUIT THERMIDOR"))
	];
	StartBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
	[
		SNew(STextBlock).Font(Font("Italic", 15)).ColorAndOpacity(FSlateColor(Ink)).AutoWrapText(true)
		.Text(LOCTEXT("Intro", "Paris, 8 thermidor an II. La Terreur est à son comble, les charrettes roulent chaque jour vers l'échafaud. Dans la salle d'un comité de surveillance, un commissaire ouvre les yeux sur un plancher sale. Il ne sait plus qui il est."))
	];
	StartBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)
	[
		SNew(STextBlock).Font(Font("Regular", 13)).ColorAndOpacity(FSlateColor(Muted)).AutoWrapText(true)
		.Text(LOCTEXT("Choose", "Choisis le commissaire que tu seras. Touches 1 à 9 pour répondre, Tab pour explorer, E pour interagir."))
	];

	const TArray<FArchetype>& Archs = S->GetArchetypes();
	for (int32 i = 0; i < Archs.Num(); ++i)
	{
		StartBox->AddSlot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SButton)
			.ContentPadding(FMargin(14.f, 10.f))
			.OnClicked_Lambda([Weak, i]() { if (Weak.IsValid()) { Weak->NewGame(i); } return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(Font("Bold", 16)).Text(FText::FromString(Archs[i].Name.ToUpper()))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Font(Font("Regular", 13)).AutoWrapText(true).Text(FText::FromString(Archs[i].Description))
				]
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
