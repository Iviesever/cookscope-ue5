#include "CookScopeEditorSession.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCookScopeEditorWorkflowTest,
	"CookScope.PACT60.EditorWorkflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	class FVerifyCookScopeEditorScan final : public IAutomationLatentCommand
	{
	public:
		FVerifyCookScopeEditorScan(
			FAutomationTestBase* InTest,
			TSharedRef<FCookScopeEditorSession> InSession,
			FString InExportPath)
			: Test(InTest), Session(MoveTemp(InSession)), ExportPath(MoveTemp(InExportPath))
		{
		}

		virtual bool Update() override
		{
			if (Session->GetState() == ECookScopeEditorSessionState::Scanning) return false;
			Test->TestEqual(TEXT("Editor scan completes"), Session->GetState(), ECookScopeEditorSessionState::Complete);
			const TArray<FCookScopeEditorFindingItem> Findings = Session->GetFilteredFindings({});
			Test->TestEqual(TEXT("Editor uses shared BadName finding"), Findings.Num(), 1);
			if (Findings.Num() == 1)
			{
				Test->TestEqual(TEXT("Finding Rule ID is shared"), Findings[0].RuleId, FString(TEXT("naming.asset-prefix")));
				Test->TestTrue(TEXT("Finding path identifies BadName"), Findings[0].AssetPath.EndsWith(TEXT("/BadName.BadName")));
				const FString FindingDetails = Session->DescribeFinding(Findings[0]);
				Test->TestTrue(TEXT("Finding expands Rule ID"), FindingDetails.Contains(TEXT("naming.asset-prefix")));
				Test->TestTrue(TEXT("Finding expands baseline state"), FindingDetails.Contains(TEXT("Baseline state Not applicable")));
			}

			FCookScopeEditorFilter ErrorFilter;
			ErrorFilter.Severity = TEXT("error");
			Test->TestEqual(TEXT("Error filter retains finding"), Session->GetFilteredFindings(ErrorFilter).Num(), 1);
			FCookScopeEditorFilter ClassFilter;
			ClassFilter.AssetClass = TEXT("/Script/Engine.Texture2D");
			Test->TestEqual(TEXT("Class filter removes DataAsset finding"), Session->GetFilteredFindings(ClassFilter).Num(), 0);
			FCookScopeEditorFilter SearchFilter;
			SearchFilter.PathSearch = TEXT("NoSuchAsset");
			Test->TestEqual(TEXT("Path search removes unmatched finding"), Session->GetFilteredFindings(SearchFilter).Num(), 0);

			const TArray<FString> Roots = {TEXT("/Game/CookScopeFixtures/Primary/DA_Primary.DA_Primary")};
			const cookscope::WhyCookedResult Why = Session->ExplainWhyCooked(
				Roots,
				TEXT("/Game/CookScopeFixtures/Targets/DA_Target.DA_Target"));
			Test->TestTrue(TEXT("Editor exposes why-cooked chain"), Why.found && !Why.steps.empty());
			const FString Details = Session->DescribeAsset(TEXT("/Game/CookScopeFixtures/Primary/DA_Primary.DA_Primary"));
			Test->TestTrue(TEXT("Asset details show Chunk"), Details.Contains(TEXT("Chunk 1")));
			Test->TestTrue(TEXT("Asset details show Bundle"), Details.Contains(TEXT("Bundle Default")));
			Test->TestTrue(TEXT("Asset details show actual Cook bytes"), Details.Contains(TEXT("Actual Cook")));
			Test->TestTrue(TEXT("Asset details expand typed dependencies"), Details.Contains(TEXT("Manage ->")));
			Test->TestTrue(TEXT("Editor exports all reports"), Session->ExportReports(ExportPath));
			for (const TCHAR* Name : {TEXT("cookscope.json"), TEXT("cookscope.sarif"), TEXT("cookscope.junit.xml"), TEXT("cookscope.html")})
			{
				Test->TestTrue(TEXT("Exported report exists"), FPaths::FileExists(ExportPath / Name));
			}

			Session->Shutdown();
			Test->TestEqual(TEXT("Shutdown returns session to Idle"), Session->GetState(), ECookScopeEditorSessionState::Idle);
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FCookScopeEditorSession> Session;
		FString ExportPath;
	};

	class FVerifyCookScopeEditorComparison final : public IAutomationLatentCommand
	{
	public:
		FVerifyCookScopeEditorComparison(FAutomationTestBase* InTest, TSharedRef<FCookScopeEditorSession> InSession)
			: Test(InTest), Session(MoveTemp(InSession))
		{
		}

		virtual bool Update() override
		{
			if (Session->GetState() == ECookScopeEditorSessionState::Scanning) return false;
			Test->TestEqual(TEXT("Editor comparison scan completes"), Session->GetState(), ECookScopeEditorSessionState::Complete);
			const FString Comparison = Session->GetComparisonSummary();
			Test->AddInfo(FString::Printf(TEXT("Comparison summary: %s"), *Comparison));
			Test->TestTrue(TEXT("Editor exposes baseline/candidate comparison"), Comparison.Contains(TEXT("Asset changes 1")));
			Test->TestTrue(TEXT("Editor comparison exposes size changes"), Comparison.Contains(TEXT("Size changes")));
			Session->Shutdown();
			return true;
		}

	private:
		FAutomationTestBase* Test;
		TSharedRef<FCookScopeEditorSession> Session;
	};
}

bool FCookScopeEditorWorkflowTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CookScope"));
	TestTrue(TEXT("CookScope plugin is available"), Plugin.IsValid());
	if (!Plugin) return false;

	FCookScopeEditorScanSettings Settings;
	Settings.Scope = TEXT("/Game/CookScopeFixtures");
	Settings.ConfigPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Config/CookScopeRules.json"));
	Settings.SourceSha = TEXT("0000000000000000000000000000000000000000");
	Settings.CookRegistryPath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Cooked/Windows/CookScopeSample/Metadata/DevelopmentAssetRegistry.bin"));
	Settings.CookPlatform = TEXT("Windows");
	Settings.CookConfiguration = TEXT("Development");

	FCookScopeEditorScanSettings BoundedSettings = Settings;
	BoundedSettings.Scope = TEXT("/Game");
	BoundedSettings.MaximumAssets = 1;
	TSharedRef<FCookScopeEditorSession> BoundedSession = MakeShared<FCookScopeEditorSession>();
	TestFalse(TEXT("Editor scan fails closed at its explicit asset bound"), BoundedSession->StartScan(BoundedSettings));
	TestEqual(TEXT("Bounded scan reports failure"), BoundedSession->GetState(), ECookScopeEditorSessionState::Failed);
	TestTrue(TEXT("Bounded scan status identifies the limit"), BoundedSession->GetStatusText().Contains(TEXT("asset limit")));
	BoundedSession->Shutdown();

	TSharedRef<FCookScopeEditorSession> Session = MakeShared<FCookScopeEditorSession>();
	TestTrue(TEXT("Editor scan starts"), Session->StartScan(Settings));
	const FString ExportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CookScopeEditorWorkflowTest/Reports"));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyCookScopeEditorScan(this, Session, ExportPath));

	FCookScopeEditorScanSettings ComparisonSettings = Settings;
	ComparisonSettings.BaselinePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
		Plugin->GetBaseDir(),
		TEXT("../../Examples/snapshots/cookscope-real-baseline.json")));
	TSharedRef<FCookScopeEditorSession> ComparisonSession = MakeShared<FCookScopeEditorSession>();
	TestTrue(TEXT("Editor comparison scan starts"), ComparisonSession->StartScan(ComparisonSettings));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyCookScopeEditorComparison(this, ComparisonSession));

	TSharedRef<FCookScopeEditorSession> Cancelled = MakeShared<FCookScopeEditorSession>();
	TestTrue(TEXT("Cancellable scan starts"), Cancelled->StartScan(Settings));
	Cancelled->Cancel();
	TestEqual(TEXT("Cancellation is immediate and explicit"), Cancelled->GetState(), ECookScopeEditorSessionState::Cancelled);
	Cancelled->Shutdown();
	return true;
}

#endif
