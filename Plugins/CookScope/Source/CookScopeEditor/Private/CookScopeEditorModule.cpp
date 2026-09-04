#include "CookScopeEditorModule.h"

#include "CookScopeEditorSession.h"
#include "SCookScopePanel.h"

#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FCookScopeEditorModule"

namespace
{
	const FName CookScopeTabName(TEXT("CookScope"));
}

FName FCookScopeEditorModule::GetTabName()
{
	return CookScopeTabName;
}

TSharedPtr<SDockTab> FCookScopeEditorModule::InvokeTab()
{
	return FGlobalTabmanager::Get()->TryInvokeTab(FTabId(CookScopeTabName));
}

void FCookScopeEditorModule::StartupModule()
{
	Session = MakeShared<FCookScopeEditorSession>();
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		CookScopeTabName,
		FOnSpawnTab::CreateRaw(this, &FCookScopeEditorModule::SpawnCookScopeTab))
		.SetDisplayName(LOCTEXT("CookScopeTabTitle", "CookScope"))
		.SetTooltipText(LOCTEXT("CookScopeTabTooltip", "Audit asset dependencies and Cook budgets."))
		.SetMenuType(ETabSpawnerMenuType::Enabled);
}

void FCookScopeEditorModule::ShutdownModule()
{
	if (const TSharedPtr<SDockTab> Existing = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(CookScopeTabName)))
	{
		Existing->RequestCloseTab();
	}
	if (Session)
	{
		Session->Shutdown();
		Session.Reset();
	}
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CookScopeTabName);
}

TSharedRef<SDockTab> FCookScopeEditorModule::SpawnCookScopeTab(const FSpawnTabArgs& SpawnTabArgs)
{
	(void)SpawnTabArgs;
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SCookScopePanel)
			.Session(Session)
		];
}

IMPLEMENT_MODULE(FCookScopeEditorModule, CookScopeEditor);

#undef LOCTEXT_NAMESPACE
