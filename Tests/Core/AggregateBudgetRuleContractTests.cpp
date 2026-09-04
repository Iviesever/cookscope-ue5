#include "cookscope/rules.h"

#include <iostream>
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

	std::string Rule(std::string_view id, std::string_view parameters)
	{
		return "{\"id\":\"" + std::string(id) +
			"\",\"name\":\"Aggregate budget\",\"description\":\"Aggregate fixture\",\"severity\":\"error\","
			"\"scope\":{\"include\":[\"/Game/**\"],\"exclude\":[]},\"parameters\":" + std::string(parameters) +
			",\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\","
			"\"helpUri\":\"docs/rules/budget.md\"}";
	}

	cookscope::AssetRecord Asset(
		std::string path,
		std::string assetClass,
		std::uint64_t packageBytes,
		std::optional<std::uint64_t> cookedBytes)
	{
		cookscope::AssetRecord asset;
		asset.objectPath = std::move(path);
		asset.assetClass = std::move(assetClass);
		asset.diskSize = {cookscope::MeasurementKind::PackageDisk, packageBytes};
		asset.cookedSize = cookedBytes
			? cookscope::SizeMeasurement{cookscope::MeasurementKind::ActualCooked, *cookedBytes}
			: cookscope::SizeMeasurement{cookscope::MeasurementKind::Unavailable, std::nullopt};
		return asset;
	}
}

int main()
{
	cookscope::Snapshot snapshot;
	snapshot.assets = {
		Asset("/Game/Art/T_A.T_A", "/Script/Engine.Texture2D", 600, 500),
		Asset("/Game/Art/T_B.T_B", "/Script/Engine.Texture2D", 500, 400),
		Asset("/Game/Audio/S_A.S_A", "/Script/Engine.SoundWave", 300, std::nullopt),
	};
	const std::string configJson = "{\"schema\":\"cookscope.rules/1\",\"rules\":[" +
		Rule("budget.directory", "{\"patterns\":[\"/Game/Art/**\"],\"budgetBytes\":1000,\"measurement\":\"package-disk\"}") + "," +
		Rule("budget.type", "{\"assetClass\":\"/Script/Engine.Texture2D\",\"budgetBytes\":1000,\"measurement\":\"package-disk\"}") + "," +
		Rule("budget.project", "{\"budgetBytes\":1300,\"measurement\":\"package-disk\"}") + "," +
		Rule("budget.project-cooked", "{\"budgetBytes\":800,\"measurement\":\"actual-cooked\"}") + "]}";
	const auto config = cookscope::ParseRuleConfig(configJson);
	if (!config.ok) return Fail("aggregate budget configuration must parse");
	const cookscope::AnalysisResult result = cookscope::Evaluate(snapshot, config.value);
	if (result.findings.size() != 3 || result.diagnostics.size() != 1)
	{
		return Fail("three complete aggregates and one incomplete Cook aggregate are expected");
	}
	for (const cookscope::Finding& finding : result.findings)
	{
		if (finding.measurementKind != cookscope::MeasurementKind::PackageDisk ||
			!finding.observedBytes.has_value() || !finding.budgetBytes.has_value())
		{
			return Fail("aggregate finding must retain measured kind, total, and budget");
		}
	}
	if (result.findings[0].ruleId != "budget.directory" || result.findings[0].observedBytes != 1100 ||
		result.findings[1].ruleId != "budget.project" || result.findings[1].observedBytes != 1400 ||
		result.findings[2].ruleId != "budget.type" || result.findings[2].observedBytes != 1100)
	{
		return Fail("directory, project, and type totals must be exact and stable");
	}
	if (result.diagnostics[0].ruleId != "budget.project-cooked" ||
		result.diagnostics[0].assetPath != "/Game/Audio/S_A.S_A" ||
		result.diagnostics[0].code != cookscope::AnalysisDiagnosticCode::MeasurementUnavailable)
	{
		return Fail("incomplete actual Cook aggregate must identify the missing asset");
	}

	std::cout << "PASS: directory, type, project, and incomplete Cook aggregate budgets\n";
	return 0;
}
