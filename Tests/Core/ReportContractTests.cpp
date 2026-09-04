#include "cookscope/reports.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace
{
	int Fail(std::string_view message)
	{
		std::cerr << "FAIL: " << message << '\n';
		return 1;
	}

	bool Write(std::string_view path, std::string_view value)
	{
		std::ofstream output(std::string(path), std::ios::binary | std::ios::trunc);
		output.write(value.data(), static_cast<std::streamsize>(value.size()));
		return static_cast<bool>(output);
	}
}

int main()
{
	cookscope::Snapshot snapshot;
	snapshot.provenance = {
		"5.8.0-55116800",
		"Windows",
		"Development",
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
	cookscope::AssetRecord asset;
	asset.objectPath = "/Game/测试/Bad</script><script>alert(1)</script>.Bad";
	asset.assetClass = "/Script/Engine.Texture2D";
	asset.primaryAssetId = "Texture:Bad";
	asset.cookedSize = {cookscope::MeasurementKind::ActualCooked, 2048};
	asset.chunkIds = {1, 2};
	asset.assetBundles = {"Default"};
	asset.dependencies.push_back({"/Game/Shared/T_Common.T_Common", cookscope::DependencyKind::Soft});
	snapshot.assets = {asset};

	const auto config = cookscope::ParseRuleConfig(
		R"({"schema":"cookscope.rules/1","rules":[{"id":"budget.asset-size","name":"Asset budget","description":"Blocking fixture","severity":"error","scope":{"include":["/Game/**"],"exclude":[]},"parameters":{"budgetBytes":1024,"measurement":"actual-cooked"},"exceptions":[],"baseline":"report-all","failThreshold":"error","helpUri":"docs/rules/budget.md"},{"id":"budget.project-cooked","name":"Project budget","description":"Non-blocking aggregate fixture","severity":"warning","scope":{"include":["/Game/**"],"exclude":[]},"parameters":{"budgetBytes":1024,"measurement":"actual-cooked"},"exceptions":[],"baseline":"report-all","failThreshold":"error","helpUri":"docs/rules/budget.md"},{"id":"path.forbidden","name":"Clean path","description":"Passing rule fixture","severity":"error","scope":{"include":["/Game/**"],"exclude":[]},"parameters":{"patterns":["/Never/**"]},"exceptions":[],"baseline":"report-all","failThreshold":"error","helpUri":"docs/rules/path.md"},{"id":"resource.texture","name":"Diagnostic fixture","description":"Invalid rule parameters","severity":"warning","scope":{"include":["/Game/**"],"exclude":[]},"parameters":{},"exceptions":[],"baseline":"report-all","failThreshold":"error","helpUri":"docs/rules/resource.md"}]})");
	if (!config.ok) return Fail("report fixture rule config must parse");

	cookscope::AnalysisResult analysis = cookscope::Evaluate(snapshot, config.value);
	if (analysis.findings.size() != 2 || analysis.diagnostics.size() != 1)
		return Fail("report fixture must contain blocking, non-blocking aggregate, clean, and diagnostic rule outcomes");
	cookscope::SnapshotDiffResult diff;
	diff.comparable = true;
	diff.assetChanges.push_back({cookscope::AssetChangeKind::Modified, asset.objectPath, asset.objectPath, {"cookedSize"}});
	diff.edgeChanges.push_back({cookscope::EdgeChangeKind::Added, asset.objectPath, "/Game/Shared/T_Common.T_Common", {}, cookscope::DependencyKind::Soft});
	diff.sizeChanges.push_back({asset.objectPath, 1024, 2048, 1024});
	diff.findingChanges.push_back({cookscope::FindingChangeKind::Added, "budget.asset-size", asset.objectPath, {}, cookscope::Severity::Error});

	const cookscope::ReportSet first = cookscope::RenderReports(snapshot, config.value, analysis, &diff);
	const cookscope::ReportSet second = cookscope::RenderReports(snapshot, config.value, analysis, &diff);
	if (first.json != second.json || first.sarif != second.sarif || first.junit != second.junit || first.html != second.html)
	{
		return Fail("all report formats must be byte-deterministic");
	}
	const auto json = cookscope::ParseJson(first.json);
	const auto sarif = cookscope::ParseJson(first.sarif);
	if (!json.ok || !sarif.ok)
	{
		return Fail("JSON and SARIF reports must be strict JSON");
	}
	if (first.json.find("\"ruleId\":\"budget.asset-size\"") == std::string::npos ||
		first.sarif.find("\"ruleId\":\"budget.asset-size\"") == std::string::npos ||
		first.junit.find("budget.asset-size") == std::string::npos ||
		first.html.find("budget.asset-size") == std::string::npos ||
		first.json.find("/Game/测试/") == std::string::npos)
	{
		return Fail("Rule ID, asset identity, and Unicode must remain consistent across reports");
	}
	if (first.sarif.find("\"version\":\"2.1.0\"") == std::string::npos ||
		first.junit.find("<testsuite tests=\"4\" failures=\"1\" errors=\"1\"") == std::string::npos ||
		first.junit.find("name=\"path.forbidden\"") == std::string::npos ||
		first.junit.find("name=\"budget.project-cooked\"") == std::string::npos ||
		first.junit.find("<error message=\"1 diagnostics\"") == std::string::npos)
	{
		return Fail("SARIF version and per-rule JUnit pass/failure/error semantics must be explicit");
	}
	if (first.html.find("https://") != std::string::npos || first.html.find("http://") != std::string::npos ||
		first.html.find("<link") != std::string::npos || first.html.find("<script src=") != std::string::npos)
	{
		return Fail("HTML report must be self-contained with no network resources");
	}
	if (first.html.find("id=\"severity-filter\"") == std::string::npos ||
		first.html.find("id=\"rule-filter\"") == std::string::npos ||
		first.html.find("id=\"class-filter\"") == std::string::npos ||
		first.html.find("id=\"path-search\"") == std::string::npos ||
		first.html.find("id=\"size-sort\"") == std::string::npos ||
		first.html.find("id=\"chunk-filter\"") == std::string::npos ||
		first.html.find("id=\"bundle-filter\"") == std::string::npos ||
		first.html.find("id=\"comparison-summary\"") == std::string::npos ||
		first.html.find("id=\"asset-table\"") == std::string::npos)
	{
		return Fail("offline HTML must expose filters, baseline comparison, dependency, and Chunk/Bundle views");
	}
	if (first.json.find("\"chunkIds\":[1,2]") == std::string::npos ||
		first.json.find("\"assetBundles\":[\"Default\"]") == std::string::npos ||
		first.json.find("\"dependencies\":[{\"kind\":\"soft\",\"target\":\"/Game/Shared/T_Common.T_Common\"}]") == std::string::npos)
	{
		return Fail("canonical JSON must expose the exact Chunk, Bundle, and dependency data used by HTML");
	}
	if (first.html.find("const findingMatches=") == std::string::npos ||
		first.html.find("visiblePaths.has(x.assetPath)") != std::string::npos)
	{
		return Fail("HTML must retain aggregate findings that do not map to a concrete asset row");
	}
	if (first.html.find("</script><script>alert(1)</script>") != std::string::npos ||
		first.html.find("\\u003c/script>\\u003cscript>alert(1)\\u003c/script>") == std::string::npos)
	{
		return Fail("embedded report data must neutralize closing-script injection");
	}
	const std::filesystem::path evidence = "Artifacts/Evidence/PACT-50/Core";
	std::error_code filesystemError;
	std::filesystem::create_directories(evidence, filesystemError);
	if (filesystemError ||
		!Write("Artifacts/Evidence/PACT-50/Core/report.json", first.json) ||
		!Write("Artifacts/Evidence/PACT-50/Core/report.sarif", first.sarif) ||
		!Write("Artifacts/Evidence/PACT-50/Core/report.junit.xml", first.junit) ||
		!Write("Artifacts/Evidence/PACT-50/Core/report.html", first.html))
	{
		return Fail("report contract evidence files must be writable under ignored Artifacts");
	}

	std::cout << "PASS: JSON, SARIF, JUnit, and self-contained HTML report contract\n";
	return 0;
}
