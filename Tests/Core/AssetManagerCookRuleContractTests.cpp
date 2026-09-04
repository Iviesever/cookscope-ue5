#include "cookscope/rules.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace
{
	int Fail(std::string_view message)
	{
		std::cerr << "FAIL: " << message << '\n';
		return 1;
	}

	std::string Rule(std::string_view id, std::string_view include, std::string_view parameters)
	{
		return "{\"id\":\"" + std::string(id) +
			"\",\"name\":\"Cook rule\",\"description\":\"Asset Manager and Cook fixture\",\"severity\":\"error\","
			"\"scope\":{\"include\":[\"" + std::string(include) + "\"],\"exclude\":[]},\"parameters\":" + std::string(parameters) +
			",\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\","
			"\"helpUri\":\"docs/rules/cook.md\"}";
	}

	cookscope::AssetRecord Asset(std::string path)
	{
		cookscope::AssetRecord asset;
		asset.objectPath = std::move(path);
		asset.cookedSize = {cookscope::MeasurementKind::Unavailable, std::nullopt};
		return asset;
	}
}

int main()
{
	using cookscope::DependencyKind;
	cookscope::Snapshot snapshot;
	cookscope::AssetRecord missing = Asset("/Game/Expected/DA_Missing.DA_Missing");
	cookscope::AssetRecord wrong = Asset("/Game/Primary/DA_Wrong.DA_Wrong");
	wrong.primaryAssetId = "WrongType:DA_Wrong";
	wrong.chunkIds = {1, 2};
	wrong.tags = {{"AlwaysCook", "true"}, {"NeverCook", "true"}};
	wrong.cookedSize = {cookscope::MeasurementKind::ActualCooked, 100};
	cookscope::AssetRecord unexpected = Asset("/Game/Unexpected/A.A");
	unexpected.cookedSize = {cookscope::MeasurementKind::ActualCooked, 50};
	cookscope::AssetRecord editor = Asset("/Game/EditorOnly/E.E");
	editor.cookedSize = {cookscope::MeasurementKind::ActualCooked, 25};
	cookscope::AssetRecord redirector = Asset("/Game/Redirectors/Old.Old");
	redirector.assetClass = "/Script/CoreUObject.ObjectRedirector";
	cookscope::AssetRecord missingReference = Asset("/Game/MissingRef/A.A");
	missingReference.dependencies = {{"/Game/DoesNotExist.X", DependencyKind::Hard}};
	cookscope::AssetRecord duplicateOne = Asset("/Game/Duplicate/One/Thing.Thing");
	cookscope::AssetRecord duplicateTwo = Asset("/Game/Duplicate/Two/Thing.Thing");
	snapshot.assets = {
		missing,
		wrong,
		unexpected,
		editor,
		redirector,
		missingReference,
		duplicateOne,
		duplicateTwo,
	};

	const std::string configJson = "{\"schema\":\"cookscope.rules/1\",\"rules\":[" +
		Rule("asset-manager.primary-required", "/Game/Expected/**", "{}") + "," +
		Rule("asset-manager.bundle-required", "/Game/Primary/**", "{\"bundle\":\"Default\"}") + "," +
		Rule("asset-manager.primary-type", "/Game/Primary/**", "{\"expectedType\":\"CookScopeFixture\"}") + "," +
		Rule("asset-manager.chunk-conflict", "/Game/Primary/**", "{\"maxChunks\":1}") + "," +
		Rule("asset-manager.chunk-required", "/Game/Primary/**", "{\"chunkId\":3}") + "," +
		Rule("cook.unexpected", "/Game/Unexpected/**", "{\"allowedPatterns\":[\"/Game/Expected/**\"]}") + "," +
		Rule("cook.missing", "/Game/Expected/**", "{\"expectedPatterns\":[\"/Game/Expected/**\"]}") + "," +
		Rule("cook.editor-only-leak", "/Game/EditorOnly/**", "{\"editorPatterns\":[\"/Game/EditorOnly/**\"]}") + "," +
		Rule("cook.rule-conflict", "/Game/Primary/**", "{}") + "," +
		Rule("redirector.present", "/Game/Redirectors/**", "{}") + "," +
		Rule("reference.missing", "/Game/MissingRef/**", "{}") + "," +
		Rule("naming.ambiguous", "/Game/Duplicate/**", "{}") + "]}";
	const auto config = cookscope::ParseRuleConfig(configJson);
	if (!config.ok)
	{
		return Fail("Asset Manager/Cook rule configuration must parse");
	}
	const cookscope::AnalysisResult result = cookscope::Evaluate(snapshot, config.value);
	if (!result.diagnostics.empty() || result.findings.size() != 12)
	{
		return Fail("each Asset Manager/Cook rule must produce one isolated finding");
	}

	std::map<std::string, int, std::less<>> counts;
	for (const cookscope::Finding& finding : result.findings) ++counts[finding.ruleId];
	for (const auto& [ruleId, count] : counts)
	{
		if (count != 1) return Fail("each isolated rule must emit exactly one finding");
		(void)ruleId;
	}
	if (counts.size() != 12)
	{
		return Fail("all twelve Asset Manager/Cook rule IDs must execute");
	}
	const auto missingRef = std::find_if(result.findings.begin(), result.findings.end(), [](const cookscope::Finding& finding) {
		return finding.ruleId == "reference.missing";
	});
	if (missingRef == result.findings.end() || missingRef->relatedAsset != "/Game/DoesNotExist.X" ||
		missingRef->dependencyKind != std::optional<DependencyKind>(DependencyKind::Hard))
	{
		return Fail("missing reference finding must preserve target and typed edge");
	}
	const auto ambiguous = std::find_if(result.findings.begin(), result.findings.end(), [](const cookscope::Finding& finding) {
		return finding.ruleId == "naming.ambiguous";
	});
	if (ambiguous == result.findings.end() || ambiguous->assetPath != "/Game/Duplicate/One/Thing.Thing" ||
		ambiguous->relatedAsset != "/Game/Duplicate/Two/Thing.Thing")
	{
		return Fail("ambiguous naming finding must choose stable first and related assets");
	}

	std::cout << "PASS: Asset Manager, Cook, redirector, missing reference, and ambiguous name rules\n";
	return 0;
}
