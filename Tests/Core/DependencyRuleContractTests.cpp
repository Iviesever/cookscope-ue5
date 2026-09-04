#include "cookscope/rules.h"

#include <iostream>
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
			"\",\"name\":\"Dependency rule\",\"description\":\"Dependency fixture\",\"severity\":\"error\","
			"\"scope\":{\"include\":[\"/Game/**\"],\"exclude\":[]},\"parameters\":" + std::string(parameters) +
			",\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\","
			"\"helpUri\":\"docs/rules/dependency.md\"}";
	}

	cookscope::AssetRecord Asset(std::string path, std::vector<cookscope::DependencyEdge> edges = {})
	{
		cookscope::AssetRecord asset;
		asset.objectPath = std::move(path);
		asset.dependencies = std::move(edges);
		return asset;
	}
}

int main()
{
	using cookscope::DependencyKind;
	cookscope::Snapshot snapshot;
	snapshot.assets = {
		Asset("/Game/Runtime/A.A", {
			{"/Game/EditorOnly/E.E", DependencyKind::Hard},
			{"/Game/Runtime/B.B", DependencyKind::Soft},
			{"/Plugin/Other/P.P", DependencyKind::Manage}}),
		Asset("/Game/Runtime/B.B", {{"/Game/Runtime/C.C", DependencyKind::Hard}}),
		Asset("/Game/Runtime/C.C"),
		Asset("/Game/EditorOnly/E.E"),
		Asset("/Game/Cycle/X.X", {{"/Game/Cycle/Y.Y", DependencyKind::Hard}}),
		Asset("/Game/Cycle/Y.Y", {{"/Game/Cycle/X.X", DependencyKind::Hard}}),
	};

	const std::string configJson = "{\"schema\":\"cookscope.rules/1\",\"rules\":[" +
		Rule("dependency.runtime-to-editor", "{\"runtimePatterns\":[\"/Game/Runtime/**\"],\"editorPatterns\":[\"/Game/EditorOnly/**\"]}") + "," +
		Rule("dependency.max-fanout", "{\"maxFanOut\":2}") + "," +
		Rule("dependency.max-depth", "{\"maxDepth\":1}") + "," +
		Rule("dependency.forbidden", "{\"from\":[\"/Game/Runtime/**\"],\"to\":[\"/Game/EditorOnly/**\"],\"kinds\":[\"hard\"]}") + "," +
		Rule("dependency.cycle", "{}") + "]}";
	const auto config = cookscope::ParseRuleConfig(configJson);
	if (!config.ok)
	{
		return Fail("dependency rule configuration must parse");
	}

	const cookscope::AnalysisResult result = cookscope::Evaluate(snapshot, config.value);
	if (!result.diagnostics.empty() || result.findings.size() != 5)
	{
		return Fail("all dependency rule families must produce one stable finding without diagnostics");
	}
	const std::vector<std::string> expectedIds = {
		"dependency.cycle",
		"dependency.forbidden",
		"dependency.max-depth",
		"dependency.max-fanout",
		"dependency.runtime-to-editor",
	};
	for (std::size_t index = 0; index < expectedIds.size(); ++index)
	{
		if (result.findings[index].ruleId != expectedIds[index])
		{
			return Fail("dependency findings must use stable Rule ID ordering");
		}
	}
	const cookscope::Finding& forbidden = result.findings[1];
	if (forbidden.assetPath != "/Game/Runtime/A.A" || forbidden.relatedAsset != "/Game/EditorOnly/E.E" ||
		forbidden.dependencyKind != std::optional<DependencyKind>(DependencyKind::Hard) ||
		forbidden.dependencyPath.size() != 1 || forbidden.dependencyPath[0].target != forbidden.relatedAsset)
	{
		return Fail("forbidden dependency finding must retain the typed evidence edge");
	}
	const cookscope::Finding& depth = result.findings[2];
	if (depth.assetPath != "/Game/Runtime/A.A" || depth.dependencyPath.size() != 2 ||
		depth.dependencyPath[0].kind != DependencyKind::Soft || depth.dependencyPath[1].kind != DependencyKind::Hard)
	{
		return Fail("maximum depth finding must retain the stable violating path");
	}

	std::cout << "PASS: dependency boundary, cycle, fan-out, and depth rule contract\n";
	return 0;
}
