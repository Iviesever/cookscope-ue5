#include "SCookScopePanel.h"

#include "Editor.h"
#include "Interfaces/IPluginManager.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SCookScopePanel"

namespace
{
	TSharedRef<SWidget> LabelledField(const FText& Label, TSharedPtr<SEditableTextBox>& Output, const FString& Value)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
			[
				SNew(SBox).WidthOverride(92.0f)
				[
					SNew(STextBlock).Text(Label).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SAssignNew(Output, SEditableTextBox).Text(FText::FromString(Value))
			];
	}
}

void SCookScopePanel::Construct(const FArguments& Arguments)
{
	Session = Arguments._Session;
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CookScope"));
	const FString ConfigPath = Plugin ? FPaths::Combine(Plugin->GetBaseDir(), TEXT("Config/CookScopeRules.json")) : FString();
	const FString CookRegistry = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Cooked/Windows/CookScopeSample/Metadata/DevelopmentAssetRegistry.bin"));
	const FString OutputPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CookScope/Reports"));

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 14, 16, 4)
			[
				SNew(STextBlock).Text(LOCTEXT("Title", "CookScope")).Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 14)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Subtitle", "Asset dependency and Cook budget audit"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 6)
			[
				LabelledField(LOCTEXT("Scope", "Scope"), ScopeInput, TEXT("/Game/CookScopeFixtures"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 6)
			[
				LabelledField(LOCTEXT("Config", "Rule config"), ConfigInput, ConfigPath)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 6)
			[
				LabelledField(LOCTEXT("SourceSha", "Source SHA"), SourceShaInput, TEXT("0000000000000000000000000000000000000000"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 6)
			[
				LabelledField(LOCTEXT("Baseline", "Baseline"), BaselineInput, FString())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 6)
			[
				LabelledField(LOCTEXT("CookRegistry", "Cook registry"), CookRegistryInput, CookRegistry)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 10)
			[
				LabelledField(LOCTEXT("Output", "Export path"), OutputInput, OutputPath)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SNew(SButton).Text(LOCTEXT("Scan", "Run scan")).OnClicked(this, &SCookScopePanel::StartScan)
					.IsEnabled_Lambda([this] { return Session && Session->GetState() != ECookScopeEditorSessionState::Scanning; })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SNew(SButton).Text(LOCTEXT("Cancel", "Cancel")).OnClicked(this, &SCookScopePanel::CancelScan)
					.IsEnabled_Lambda([this] { return Session && Session->GetState() == ECookScopeEditorSessionState::Scanning; })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("Export", "Export reports")).OnClicked(this, &SCookScopePanel::ExportReports)
					.IsEnabled_Lambda([this] { return Session && Session->GetState() == ECookScopeEditorSessionState::Complete; })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 4)
			[
				SNew(SProgressBar).Percent_Lambda([this] { return Session ? TOptional<float>(Session->GetProgress()) : TOptional<float>(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 14)
			[
				SNew(STextBlock).Text_Lambda([this] { return FText::FromString(Session ? Session->GetStatusText() : TEXT("Session unavailable")); })
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 8)
			[
				SNew(SGridPanel).FillColumn(0, 1.0f).FillColumn(1, 1.0f).FillColumn(2, 1.0f).FillColumn(3, 1.0f)
				+ SGridPanel::Slot(0, 0).Padding(0, 0, 6, 0)[SAssignNew(SeverityFilter, SEditableTextBox).HintText(LOCTEXT("SeverityFilter", "Severity")).OnTextChanged(this, &SCookScopePanel::HandleFilterChanged)]
				+ SGridPanel::Slot(1, 0).Padding(0, 0, 6, 0)[SAssignNew(RuleFilter, SEditableTextBox).HintText(LOCTEXT("RuleFilter", "Rule ID")).OnTextChanged(this, &SCookScopePanel::HandleFilterChanged)]
				+ SGridPanel::Slot(2, 0).Padding(0, 0, 6, 0)[SAssignNew(ClassFilter, SEditableTextBox).HintText(LOCTEXT("ClassFilter", "Asset class")).OnTextChanged(this, &SCookScopePanel::HandleFilterChanged)]
				+ SGridPanel::Slot(3, 0)[SAssignNew(PathFilter, SEditableTextBox).HintText(LOCTEXT("PathFilter", "Path search")).OnTextChanged(this, &SCookScopePanel::HandleFilterChanged)]
			]
			+ SVerticalBox::Slot().MinHeight(210.0f).Padding(16, 0, 16, 10)
			[
				SAssignNew(FindingList, SListView<TSharedPtr<FCookScopeEditorFindingItem>>)
				.ListItemsSource(&Rows)
				.OnGenerateRow(this, &SCookScopePanel::GenerateFindingRow)
				.OnSelectionChanged(this, &SCookScopePanel::HandleSelectionChanged)
				.SelectionMode(ESelectionMode::Single)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 6, 0)[SAssignNew(RootInput, SEditableTextBox).HintText(LOCTEXT("RootHint", "Cook root(s), comma separated")).Text(FText::FromString(TEXT("/Game/CookScopeFixtures/Primary/DA_Primary.DA_Primary")))]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 6, 0)[SAssignNew(TargetInput, SEditableTextBox).HintText(LOCTEXT("TargetHint", "Why-cooked target")).Text(FText::FromString(TEXT("/Game/CookScopeFixtures/Targets/DA_Target.DA_Target")))]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(LOCTEXT("Explain", "Explain why cooked")).OnClicked(this, &SCookScopePanel::ExplainWhyCooked)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(16, 0, 16, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)[SNew(SButton).Text(LOCTEXT("Locate", "Locate in Content Browser")).OnClicked(this, &SCookScopePanel::LocateAsset)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(LOCTEXT("Open", "Open asset")).OnClicked(this, &SCookScopePanel::OpenAsset)]
			]
			+ SVerticalBox::Slot().MinHeight(130.0f).Padding(16, 0, 16, 16)
			[
				SAssignNew(Details, SMultiLineEditableTextBox).IsReadOnly(true).Text(LOCTEXT("DetailsEmpty", "Select a finding or request a why-cooked explanation."))
			]
		]
	];
	RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateSP(this, &SCookScopePanel::UpdateFromSession));
}

FReply SCookScopePanel::StartScan()
{
	if (!Session) return FReply::Handled();
	FCookScopeEditorScanSettings Settings;
	Settings.Scope = ScopeInput->GetText().ToString();
	Settings.ConfigPath = ConfigInput->GetText().ToString();
	Settings.SourceSha = SourceShaInput->GetText().ToString();
	Settings.BaselinePath = BaselineInput->GetText().ToString();
	Settings.CookRegistryPath = CookRegistryInput->GetText().ToString();
	Session->StartScan(Settings);
	RefreshRows();
	return FReply::Handled();
}

FReply SCookScopePanel::CancelScan()
{
	if (Session) Session->Cancel();
	return FReply::Handled();
}

FReply SCookScopePanel::ExportReports()
{
	if (Session)
	{
		const bool Exported = Session->ExportReports(OutputInput->GetText().ToString());
		Details->SetText(Exported
			? LOCTEXT("ExportSucceeded", "JSON, SARIF, JUnit, and HTML reports exported.")
			: LOCTEXT("ExportFailed", "Report export failed. Check the session status and output path."));
	}
	return FReply::Handled();
}

FReply SCookScopePanel::ExplainWhyCooked()
{
	if (!Session) return FReply::Handled();
	TArray<FString> Roots;
	RootInput->GetText().ToString().ParseIntoArray(Roots, TEXT(","), true);
	for (FString& Root : Roots) Root.TrimStartAndEndInline();
	const cookscope::WhyCookedResult Why = Session->ExplainWhyCooked(Roots, TargetInput->GetText().ToString());
	FString Text = Why.found ? FString::Printf(TEXT("Root %s"), *FString(UTF8_TO_TCHAR(Why.root.c_str()))) : TEXT("No complete path found");
	for (const cookscope::DependencyStep& Step : Why.steps)
	{
		Text += FString::Printf(TEXT("\n%s -> %s (kind %d)"), UTF8_TO_TCHAR(Step.source.c_str()), UTF8_TO_TCHAR(Step.target.c_str()), static_cast<int32>(Step.kind));
	}
	Details->SetText(FText::FromString(Text));
	return FReply::Handled();
}

FReply SCookScopePanel::LocateAsset()
{
	if (!Selected || !GEditor) return FReply::Handled();
	if (UObject* Asset = LoadObject<UObject>(nullptr, *Selected->AssetPath))
	{
		TArray<UObject*> Assets = {Asset};
		GEditor->SyncBrowserToObjects(Assets);
	}
	return FReply::Handled();
}

FReply SCookScopePanel::OpenAsset()
{
	if (!Selected || !GEditor) return FReply::Handled();
	if (UObject* Asset = LoadObject<UObject>(nullptr, *Selected->AssetPath))
	{
		if (UAssetEditorSubsystem* Editors = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()) Editors->OpenEditorForAsset(Asset);
	}
	return FReply::Handled();
}

void SCookScopePanel::RefreshRows()
{
	Rows.Reset();
	if (Session)
	{
		FCookScopeEditorFilter Filter;
		Filter.Severity = SeverityFilter->GetText().ToString();
		Filter.RuleId = RuleFilter->GetText().ToString();
		Filter.AssetClass = ClassFilter->GetText().ToString();
		Filter.PathSearch = PathFilter->GetText().ToString();
		for (FCookScopeEditorFindingItem& Item : Session->GetFilteredFindings(Filter)) Rows.Add(MakeShared<FCookScopeEditorFindingItem>(MoveTemp(Item)));
	}
	if (FindingList) FindingList->RequestListRefresh();
}

void SCookScopePanel::HandleFilterChanged(const FText& Text)
{
	(void)Text;
	RefreshRows();
}

void SCookScopePanel::HandleSelectionChanged(TSharedPtr<FCookScopeEditorFindingItem> Item, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	Selected = MoveTemp(Item);
	if (Selected && Session) Details->SetText(FText::FromString(Session->DescribeAsset(Selected->AssetPath)));
}

TSharedRef<ITableRow> SCookScopePanel::GenerateFindingRow(
	TSharedPtr<FCookScopeEditorFindingItem> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FCookScopeEditorFindingItem>>, OwnerTable)
		.Padding(4.0f)
		[
			SNew(SGridPanel).FillColumn(2, 1.0f)
			+ SGridPanel::Slot(0, 0).Padding(0, 0, 10, 0)[SNew(STextBlock).Text(FText::FromString(Item->Severity))]
			+ SGridPanel::Slot(1, 0).Padding(0, 0, 10, 0)[SNew(STextBlock).Text(FText::FromString(Item->RuleId))]
			+ SGridPanel::Slot(2, 0).Padding(0, 0, 10, 0)[SNew(STextBlock).Text(FText::FromString(Item->AssetPath))]
			+ SGridPanel::Slot(3, 0)[SNew(STextBlock).Text(FText::FromString(Item->Message))]
		];
}

EActiveTimerReturnType SCookScopePanel::UpdateFromSession(double CurrentTime, float DeltaTime)
{
	(void)CurrentTime;
	(void)DeltaTime;
	if (Session)
	{
		const ECookScopeEditorSessionState State = Session->GetState();
		if (State != LastState)
		{
			LastState = State;
			RefreshRows();
		}
	}
	return EActiveTimerReturnType::Continue;
}

#undef LOCTEXT_NAMESPACE
