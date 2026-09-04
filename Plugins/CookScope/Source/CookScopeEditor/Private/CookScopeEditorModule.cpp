#include "CookScopeEditorModule.h"

#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

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
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		CookScopeTabName,
		FOnSpawnTab::CreateRaw(this, &FCookScopeEditorModule::SpawnCookScopeTab))
		.SetDisplayName(LOCTEXT("CookScopeTabTitle", "CookScope"))
		.SetTooltipText(LOCTEXT("CookScopeTabTooltip", "Audit asset dependencies and Cook budgets."))
		.SetMenuType(ETabSpawnerMenuType::Enabled);
}

void FCookScopeEditorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CookScopeTabName);
}

TSharedRef<SDockTab> FCookScopeEditorModule::SpawnCookScopeTab(const FSpawnTabArgs& SpawnTabArgs)
{
	(void)SpawnTabArgs;
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBorder)
			.Padding(16.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BootstrapMessage", "CookScope is loaded. Asset audit controls are added in subsequent verified slices."))
			]
		];
}

IMPLEMENT_MODULE(FCookScopeEditorModule, CookScopeEditor);

#undef LOCTEXT_NAMESPACE
