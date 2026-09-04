#include "CookScopeAuditCommandlet.h"
#include "CookScopeEditorModule.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/AutomationTest.h"
#include "Widgets/Docking/SDockTab.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCookScopePACT00SmokeTest,
	"CookScope.PACT00.EditorAndCommandletContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCookScopePACT00SmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("The public tab identifier is stable"), FCookScopeEditorModule::GetTabName(), FName(TEXT("CookScope")));
	TestNotNull(TEXT("The audit commandlet class is registered"), UCookScopeAuditCommandlet::StaticClass());
	const TSharedPtr<SDockTab> CookScopeTab = FCookScopeEditorModule::InvokeTab();
	TestTrue(TEXT("The registered CookScope tab can be invoked"), CookScopeTab.IsValid());
	if (CookScopeTab.IsValid())
	{
		TestEqual(TEXT("The spawned tab has nomad role"), CookScopeTab->GetTabRole(), ETabRole::NomadTab);
		CookScopeTab->RequestCloseTab();
	}
	return true;
}

#endif
