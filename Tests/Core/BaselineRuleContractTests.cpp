#include "cookscope/rules.h"

#include <algorithm>
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

	std::string Rule(std::string_view id, std::string_view include, std::string_view parameters, std::string_view baseline)
	{
		return "{\"id\":\"" + std::string(id) +
			"\",\"name\":\"Baseline rule\",\"description\":\"Baseline fixture\",\"severity\":\"error\","
			"\"scope\":{\"include\":[\"" + std::string(include) + "\"],\"exclude\":[]},\"parameters\":" + std::string(parameters) +
			",\"exceptions\":[],\"baseline\":\"" + std::string(baseline) + "\",\"failThreshold\":\"error\","
			"\"helpUri\":\"docs/rules/baseline.md\"}";
	}

	cookscope::AssetRecord Texture(std::string path, std::uint64_t bytes, cookscope::DependencyKind kind)
	{
		cookscope::AssetRecord asset;
		asset.objectPath = std::move(path);
		asset.assetClass = "/Script/Engine.Texture2D";
		asset.diskSize = {cookscope::MeasurementKind::PackageDisk, bytes};
		asset.dependencies = {{"/Game/Target/T.T", kind}};
		return asset;
	}
}

int main()
{
	using cookscope::DependencyKind;
	using cookscope::FindingBaselineState;
	cookscope::Snapshot baseline;
	baseline.assets = {
		Texture("/Game/Bad/BadA.BadA", 1500, DependencyKind::Soft),
		Texture("/Game/Target/T.T", 10, DependencyKind::Soft),
	};
	cookscope::Snapshot candidate;
	candidate.assets = {
		Texture("/Game/Bad/BadA.BadA", 2000, DependencyKind::Hard),
		Texture("/Game/Bad/BadNew.BadNew", 100, DependencyKind::Soft),
		Texture("/Game/Target/T.T", 10, DependencyKind::Soft),
	};
	const std::string configJson = "{\"schema\":\"cookscope.rules/1\",\"rules\":[" +
		Rule("naming.asset-prefix", "/Game/Bad/**", "{\"classPrefixes\":{\"/Script/Engine.Texture2D\":\"T_\"}}", "report-new-or-worsened") + "," +
		Rule("budget.asset-size", "/Game/Bad/BadA*", "{\"budgetBytes\":1000,\"measurement\":\"package-disk\"}", "report-new-or-worsened") + "," +
		Rule("dependency.soft-hardened", "/Game/Bad/BadA*", "{}", "report-new-or-worsened") + "," +
		Rule("path.forbidden", "/Game/Bad/BadA*", "{\"patterns\":[\"/Game/Bad/**\"]}", "report-all") + "]}";
	const auto config = cookscope::ParseRuleConfig(configJson);
	if (!config.ok) return Fail("baseline rule configuration must parse");

	const cookscope::AnalysisResult result = cookscope::Evaluate(candidate, config.value, &baseline);
	if (!result.diagnostics.empty() || result.findings.size() != 4)
	{
		return Fail("baseline policy must retain exactly new, worsened, and report-all findings");
	}
	const auto naming = std::find_if(result.findings.begin(), result.findings.end(), [](const cookscope::Finding& finding) {
		return finding.ruleId == "naming.asset-prefix";
	});
	if (naming == result.findings.end() || naming->assetPath != "/Game/Bad/BadNew.BadNew" ||
		naming->baselineState != FindingBaselineState::New)
	{
		return Fail("existing naming violation must be suppressed while new violation remains");
	}
	const auto budget = std::find_if(result.findings.begin(), result.findings.end(), [](const cookscope::Finding& finding) {
		return finding.ruleId == "budget.asset-size";
	});
	if (budget == result.findings.end() || budget->baselineState != FindingBaselineState::Worsened ||
		budget->observedBytes != std::optional<std::uint64_t>(2000))
	{
		return Fail("increased size violation must be marked worsened");
	}
	const auto hardened = std::find_if(result.findings.begin(), result.findings.end(), [](const cookscope::Finding& finding) {
		return finding.ruleId == "dependency.soft-hardened";
	});
	if (hardened == result.findings.end() || hardened->baselineState != FindingBaselineState::Worsened ||
		hardened->dependencyKind != std::optional<DependencyKind>(DependencyKind::Hard))
	{
		return Fail("Soft to Hard edge change must be a typed worsened finding");
	}
	const auto forbidden = std::find_if(result.findings.begin(), result.findings.end(), [](const cookscope::Finding& finding) {
		return finding.ruleId == "path.forbidden";
	});
	if (forbidden == result.findings.end() || forbidden->baselineState != FindingBaselineState::Existing)
	{
		return Fail("report-all must retain unchanged baseline findings as existing");
	}

	std::cout << "PASS: baseline suppression, worsening, and Soft-to-Hard contract\n";
	return 0;
}
