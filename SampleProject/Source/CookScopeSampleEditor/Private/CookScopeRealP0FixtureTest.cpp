#include "CookScopeAssetScanner.h"
#include "CookScopeCookSnapshotReader.h"

#include "cookscope/rule_config.h"
#include "cookscope/rules.h"

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <string_view>

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCookScopeRealP0FixtureTest,
	"CookScope.PACT30.RealP0Fixtures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	std::string P0Rule(std::string_view Id, std::string_view Include, std::string_view Parameters)
	{
		return "{\"id\":\"" + std::string(Id) +
			"\",\"name\":\"Real P0 fixture\",\"description\":\"Real UE Sample Project rule fixture\",\"severity\":\"error\","
			"\"scope\":{\"include\":[\"" + std::string(Include) + "\"],\"exclude\":[]},\"parameters\":" + std::string(Parameters) +
			",\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\","
			"\"helpUri\":\"docs/RULE_MODEL.md\"}";
	}

	std::string P0Config(const std::vector<std::string>& Rules)
	{
		std::string Json = "{\"schema\":\"cookscope.rules/1\",\"rules\":[";
		for (std::size_t Index = 0; Index < Rules.size(); ++Index)
		{
			if (Index != 0) Json += ',';
			Json += Rules[Index];
		}
		return Json + "]}";
	}
}

bool FCookScopeRealP0FixtureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FCookScopeScanResult Scan = FCookScopeAssetScanner::ScanPath(
		TEXT("/Game"),
		TEXT("0000000000000000000000000000000000000000"));
	TestTrue(TEXT("Real P0 Asset Registry scan succeeds"), Scan.bSuccess);
	if (!Scan.bSuccess) return false;
	TestEqual(TEXT("All real graph/rule/resource fixtures are present"), static_cast<int32>(Scan.Snapshot.assets.size()), 20);

	const FString CookRegistry = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Cooked/Windows/CookScopeSample/Metadata/DevelopmentAssetRegistry.bin"));
	const FCookScopeCookMergeResult Merge = FCookScopeCookSnapshotReader::MergeDevelopmentRegistry(
		CookRegistry,
		TEXT("Windows"),
		TEXT("Development"),
		Scan.Snapshot);
	TestTrue(TEXT("Real P0 fixtures merge real Cook membership"), Merge.bSuccess);
	if (!Merge.bSuccess) return false;

	const std::vector<std::string> NegativeRules = {
		P0Rule("naming.asset-prefix", "/Game/CookScopeFixtures/Naming/**", R"({"classPrefixes":{"/Script/CookScopeSample.CookScopeFixtureAsset":"ZZ_"}})"),
		P0Rule("path.forbidden", "/Game/CookScopeP0Fixtures/EditorOnly/**", R"({"patterns":["/Game/CookScopeP0Fixtures/EditorOnly/**"]})"),
		P0Rule("dependency.forbidden", "/Game/CookScopeP0Fixtures/Runtime/**", R"({"from":["/Game/CookScopeP0Fixtures/Runtime/**"],"to":["/Game/CookScopeP0Fixtures/EditorOnly/**"],"kinds":["hard"]})"),
		P0Rule("dependency.runtime-to-editor", "/Game/CookScopeP0Fixtures/Runtime/**", R"({"runtimePatterns":["/Game/CookScopeP0Fixtures/Runtime/**"],"editorPatterns":["/Game/CookScopeP0Fixtures/EditorOnly/**"]})"),
		P0Rule("dependency.cycle", "/Game/CookScopeFixtures/Cycle/**", "{}"),
		P0Rule("dependency.max-fanout", "/Game/CookScopeFixtures/Primary/DA_Primary.**", R"({"maxFanOut":1})"),
		P0Rule("dependency.max-depth", "/Game/CookScopeFixtures/Primary/DA_Primary.**", R"({"maxDepth":1})"),
		P0Rule("asset-manager.primary-required", "/Game/CookScopeFixtures/Targets/**", "{}"),
		P0Rule("asset-manager.bundle-required", "/Game/CookScopeP0Fixtures/Primary/DA_Conflict.**", R"({"bundle":"Missing"})"),
		P0Rule("asset-manager.primary-type", "/Game/CookScopeP0Fixtures/Primary/DA_Conflict.**", R"({"expectedType":"WrongType"})"),
		P0Rule("asset-manager.chunk-conflict", "/Game/CookScopeP0Fixtures/Primary/DA_Conflict.**", R"({"maxChunks":0})"),
		P0Rule("asset-manager.chunk-required", "/Game/CookScopeP0Fixtures/Primary/DA_Conflict.**", R"({"chunkId":2})"),
		P0Rule("cook.unexpected", "/Game/CookScopeP0Fixtures/EditorOnly/**", R"({"allowedPatterns":["/Game/Never/**"]})"),
		P0Rule("cook.missing", "/Game/CookScopeP0Fixtures/MissingRef/**", R"({"expectedPatterns":["/Game/CookScopeP0Fixtures/MissingRef/**"]})"),
		P0Rule("cook.editor-only-leak", "/Game/CookScopeP0Fixtures/EditorOnly/**", R"({"editorPatterns":["/Game/CookScopeP0Fixtures/EditorOnly/**"]})"),
		P0Rule("cook.rule-conflict", "/Game/CookScopeP0Fixtures/Primary/DA_Conflict.**", "{}"),
		P0Rule("redirector.present", "/Game/CookScopeP0Fixtures/Redirectors/**", "{}"),
		P0Rule("reference.missing", "/Game/CookScopeP0Fixtures/MissingRef/**", "{}"),
		P0Rule("naming.ambiguous", "/Game/CookScopeP0Fixtures/Duplicate/**", "{}"),
		P0Rule("budget.asset-size", "/Game/CookScopeFixtures/Targets/**", R"({"budgetBytes":1,"measurement":"package-disk"})"),
		P0Rule("budget.directory", "/Game/CookScopeFixtures/**", R"({"patterns":["/Game/CookScopeFixtures/**"],"budgetBytes":1,"measurement":"package-disk"})"),
		P0Rule("budget.type", "/Game/CookScopeFixtures/**", R"({"assetClass":"/Script/CookScopeSample.CookScopeFixtureAsset","budgetBytes":1,"measurement":"package-disk"})"),
		P0Rule("budget.project-cooked", "/Game/CookScopeFixtures/**", R"({"budgetBytes":1,"measurement":"package-disk"})"),
	};
	const cookscope::RuleConfigParseResult NegativeConfig = cookscope::ParseRuleConfig(P0Config(NegativeRules));
	TestTrue(TEXT("Real negative P0 config parses"), NegativeConfig.ok);
	if (!NegativeConfig.ok) return false;
	const cookscope::AnalysisResult Negative = cookscope::Evaluate(Merge.Snapshot, NegativeConfig.value);
	for (const cookscope::AnalysisDiagnostic& Diagnostic : Negative.diagnostics)
	{
		AddInfo(FString::Printf(TEXT("P0 diagnostic %s: %s"), UTF8_TO_TCHAR(Diagnostic.ruleId.c_str()), UTF8_TO_TCHAR(Diagnostic.message.c_str())));
	}
	TestEqual(TEXT("Real negative P0 fixtures have no unavailable diagnostics"), static_cast<int32>(Negative.diagnostics.size()), 0);
	std::set<std::string, std::less<>> FindingRules;
	for (const cookscope::Finding& Finding : Negative.findings) FindingRules.insert(Finding.ruleId);
	for (const std::string& RuleId : FindingRules) AddInfo(FString::Printf(TEXT("P0 finding rule: %s"), UTF8_TO_TCHAR(RuleId.c_str())));
	const std::set<std::string, std::less<>> ExpectedRuleIds = {
		"asset-manager.bundle-required", "asset-manager.chunk-conflict", "asset-manager.chunk-required",
		"asset-manager.primary-required", "asset-manager.primary-type", "budget.asset-size", "budget.directory",
		"budget.project-cooked", "budget.type", "cook.editor-only-leak", "cook.missing", "cook.rule-conflict",
		"cook.unexpected", "dependency.cycle", "dependency.forbidden", "dependency.max-depth",
		"dependency.max-fanout", "dependency.runtime-to-editor", "naming.ambiguous", "naming.asset-prefix",
		"path.forbidden", "redirector.present", "reference.missing"};
	TestTrue(TEXT("Every P0 family has a real failing Sample fixture"), FindingRules == ExpectedRuleIds);
	const auto MissingReference = std::find_if(Negative.findings.begin(), Negative.findings.end(), [](const cookscope::Finding& Finding) {
		return Finding.ruleId == "reference.missing" && Finding.relatedAsset.find("DoesNotExist") != std::string::npos;
	});
	TestTrue(TEXT("Real missing-reference fixture preserves unresolved identity"), MissingReference != Negative.findings.end());

	const std::vector<std::string> PositiveRules = {
		P0Rule("naming.asset-prefix", "/Game/CookScopeFixtures/Targets/**", R"({"classPrefixes":{"/Script/CookScopeSample.CookScopeFixtureAsset":"DA_"}})"),
		P0Rule("asset-manager.bundle-required", "/Game/CookScopeFixtures/Primary/DA_Primary.**", R"({"bundle":"Default"})"),
		P0Rule("asset-manager.primary-type", "/Game/CookScopeFixtures/Primary/DA_Primary.**", R"({"expectedType":"CookScopeFixture"})"),
		P0Rule("asset-manager.chunk-required", "/Game/CookScopeFixtures/Primary/DA_Primary.**", R"({"chunkId":1})"),
		P0Rule("path.forbidden", "/Game/CookScopeFixtures/Targets/**", R"({"patterns":["/Game/Never/**"]})"),
	};
	const cookscope::RuleConfigParseResult PositiveConfig = cookscope::ParseRuleConfig(P0Config(PositiveRules));
	TestTrue(TEXT("Real positive P0 config parses"), PositiveConfig.ok);
	if (!PositiveConfig.ok) return false;
	const cookscope::AnalysisResult Positive = cookscope::Evaluate(Merge.Snapshot, PositiveConfig.value);
	TestEqual(TEXT("Real legal P0 fixtures produce no findings"), static_cast<int32>(Positive.findings.size()), 0);
	TestEqual(TEXT("Real legal P0 fixtures produce no diagnostics"), static_cast<int32>(Positive.diagnostics.size()), 0);
	return true;
}

#endif
