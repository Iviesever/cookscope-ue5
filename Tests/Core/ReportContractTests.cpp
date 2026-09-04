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
	asset.cookedSize = {cookscope::MeasurementKind::ActualCooked, 2048};
	snapshot.assets = {asset};

	const auto config = cookscope::ParseRuleConfig(
		R"({"schema":"cookscope.rules/1","rules":[{"id":"budget.asset-size","name":"Asset budget","description":"Budget fixture","severity":"error","scope":{"include":["/Game/**"],"exclude":[]},"parameters":{"budgetBytes":1024,"measurement":"actual-cooked"},"exceptions":[],"baseline":"report-all","failThreshold":"error","helpUri":"docs/rules/budget.md"}]})");
	if (!config.ok) return Fail("report fixture rule config must parse");

	cookscope::AnalysisResult analysis = cookscope::Evaluate(snapshot, config.value);
	if (analysis.findings.size() != 1) return Fail("report fixture must contain one finding");
	cookscope::SnapshotDiffResult diff;
	diff.comparable = true;
	diff.sizeChanges.push_back({asset.objectPath, 1024, 2048, 1024});

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
		first.junit.find("<testsuite tests=\"1\" failures=\"1\" errors=\"0\"") == std::string::npos)
	{
		return Fail("SARIF version and JUnit failure semantics must be explicit");
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
		first.html.find("id=\"size-sort\"") == std::string::npos)
	{
		return Fail("offline HTML must expose required filter, search, and size controls");
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
