#pragma once

#include "CookScopeEditorSession.h"

#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class SMultiLineEditableTextBox;
template <typename ItemType> class SListView;

class SCookScopePanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCookScopePanel) {}
		SLATE_ARGUMENT(TSharedPtr<FCookScopeEditorSession>, Session)
	SLATE_END_ARGS()

	void Construct(const FArguments& Arguments);

private:
	FReply StartScan();
	FReply CancelScan();
	FReply ExportReports();
	FReply ExplainWhyCooked();
	FReply LocateAsset();
	FReply OpenAsset();
	void RefreshRows();
	void HandleFilterChanged(const FText& Text);
	void HandleSelectionChanged(TSharedPtr<FCookScopeEditorFindingItem> Item, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> GenerateFindingRow(
		TSharedPtr<FCookScopeEditorFindingItem> Item,
		const TSharedRef<STableViewBase>& OwnerTable);
	EActiveTimerReturnType UpdateFromSession(double CurrentTime, float DeltaTime);

	TSharedPtr<FCookScopeEditorSession> Session;
	TSharedPtr<SEditableTextBox> ScopeInput;
	TSharedPtr<SEditableTextBox> ConfigInput;
	TSharedPtr<SEditableTextBox> SourceShaInput;
	TSharedPtr<SEditableTextBox> BaselineInput;
	TSharedPtr<SEditableTextBox> CookRegistryInput;
	TSharedPtr<SEditableTextBox> OutputInput;
	TSharedPtr<SEditableTextBox> SeverityFilter;
	TSharedPtr<SEditableTextBox> RuleFilter;
	TSharedPtr<SEditableTextBox> ClassFilter;
	TSharedPtr<SEditableTextBox> PathFilter;
	TSharedPtr<SEditableTextBox> RootInput;
	TSharedPtr<SEditableTextBox> TargetInput;
	TSharedPtr<SMultiLineEditableTextBox> Details;
	TSharedPtr<SListView<TSharedPtr<FCookScopeEditorFindingItem>>> FindingList;
	TArray<TSharedPtr<FCookScopeEditorFindingItem>> Rows;
	TSharedPtr<FCookScopeEditorFindingItem> Selected;
	ECookScopeEditorSessionState LastState = ECookScopeEditorSessionState::Idle;
};

