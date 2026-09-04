#include "cookscope/rules.h"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace
{
	int Fail(std::string_view message)
	{
		std::cerr << "FAIL: " << message << '\n';
		return 1;
	}

	std::string Rule(std::string_view id, std::string_view parameters, std::string_view exclude = "/Game/Developers/**")
	{
		return "{\"id\":\"" + std::string(id) +
			"\",\"name\":\"Test rule\",\"description\":\"Rule engine fixture\",\"severity\":\"error\","
			"\"scope\":{\"include\":[\"/Game/**\"],\"exclude\":[\"" + std::string(exclude) +
			"\"]},\"parameters\":" + std::string(parameters) +
			",\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\","
			"\"helpUri\":\"docs/rules/test.md\"}";
	}

	cookscope::AssetRecord Texture(std::string path, std::uint64_t packageBytes)
	{
		cookscope::AssetRecord asset;
		asset.objectPath = std::move(path);
		asset.assetClass = "/Script/Engine.Texture2D";
		asset.diskSize = {cookscope::MeasurementKind::PackageDisk, packageBytes};
		asset.cookedSize = {cookscope::MeasurementKind::Unavailable, std::nullopt};
		return asset;
	}
}

int main()
{
	cookscope::Snapshot snapshot;
	snapshot.assets = {
		Texture("/Game/UI/T_Good.T_Good", 128),
		Texture("/Game/Temp/BadTexture.BadTexture", 2048),
		Texture("/Game/Developers/Ivy/BadIgnored.BadIgnored", 4096),
	};
	const std::string rulesJson = "{\"schema\":\"cookscope.rules/1\",\"rules\":[" +
		Rule("path.forbidden", "{\"patterns\":[\"/Game/Temp/**\"]}") + "," +
		Rule("naming.asset-prefix", "{\"classPrefixes\":{\"/Script/Engine.Texture2D\":\"T_\"}}") + "," +
		Rule("budget.asset-size", "{\"budgetBytes\":1024,\"measurement\":\"package-disk\"}") + "," +
		Rule("budget.actual-cooked", "{\"budgetBytes\":1,\"measurement\":\"actual-cooked\"}") + "]}";
	const auto config = cookscope::ParseRuleConfig(rulesJson);
	if (!config.ok)
	{
		return Fail("rule engine fixture configuration must parse");
	}

	const cookscope::AnalysisResult result = cookscope::Evaluate(snapshot, config.value);
	if (result.findings.size() != 3 ||
		result.findings[0].ruleId != "budget.asset-size" ||
		result.findings[1].ruleId != "naming.asset-prefix" ||
		result.findings[2].ruleId != "path.forbidden")
	{
		return Fail("findings must be complete and stable by Rule ID");
	}
	if (result.findings[0].assetPath != "/Game/Temp/BadTexture.BadTexture" ||
		result.findings[0].measurementKind != cookscope::MeasurementKind::PackageDisk ||
		result.findings[0].observedBytes != std::optional<std::uint64_t>(2048) ||
		result.findings[0].budgetBytes != std::optional<std::uint64_t>(1024))
	{
		return Fail("size finding must preserve measured kind, observed bytes, and budget");
	}
	if (result.diagnostics.size() != 2 || result.diagnostics[0].ruleId != "budget.actual-cooked" ||
		result.diagnostics[0].assetPath != "/Game/Temp/BadTexture.BadTexture" ||
		result.diagnostics[0].code != cookscope::AnalysisDiagnosticCode::MeasurementUnavailable)
	{
		return Fail("unavailable actual Cook measurements must be explicit and stable");
	}
	for (const cookscope::Finding& finding : result.findings)
	{
		if (finding.assetPath.find("/Game/Developers/") == 0)
		{
			return Fail("exclude selectors must win over includes");
		}
	}

	std::cout << "PASS: shared naming, path, budget, scope, and measurement rule engine contract\n";
	return 0;
}
