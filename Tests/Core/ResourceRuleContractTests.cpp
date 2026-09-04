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

	std::string Rule(std::string_view id, std::string_view parameters)
	{
		return "{\"id\":\"" + std::string(id) +
			"\",\"name\":\"Resource rule\",\"description\":\"Resource metadata fixture\",\"severity\":\"warning\","
			"\"scope\":{\"include\":[\"/Game/**\"],\"exclude\":[]},\"parameters\":" + std::string(parameters) +
			",\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\","
			"\"helpUri\":\"docs/rules/resource.md\"}";
	}

	cookscope::AssetRecord Asset(std::string path, std::string assetClass, std::map<std::string, std::string, std::less<>> tags)
	{
		cookscope::AssetRecord asset;
		asset.objectPath = std::move(path);
		asset.assetClass = std::move(assetClass);
		asset.tags = std::move(tags);
		return asset;
	}
}

int main()
{
	cookscope::Snapshot snapshot;
	snapshot.assets = {
		Asset("/Game/Resources/T_Bad.T_Bad", "/Script/Engine.Texture2D", {
			{"TextureFormat", "PF_B8G8R8A8"}, {"TextureHeight", "4096"}, {"TextureMips", "1"}, {"TextureWidth", "4096"}}),
		Asset("/Game/Resources/SM_Bad.SM_Bad", "/Script/Engine.StaticMesh", {{"MeshTriangles", "200000"}}),
		Asset("/Game/Resources/SK_Bad.SK_Bad", "/Script/Engine.SkeletalMesh", {{"MeshVertices", "50000"}}),
		Asset("/Game/Resources/S_Bad.S_Bad", "/Script/Engine.SoundWave", {
			{"SoundDurationMs", "120000"}, {"SoundFormat", "PCM"}}),
	};

	const std::string configJson = "{\"schema\":\"cookscope.rules/1\",\"rules\":[" +
		Rule("resource.texture", "{\"maxWidth\":2048,\"maxHeight\":2048,\"minMips\":2,\"allowedFormats\":[\"BC7\"]}") + "," +
		Rule("resource.static-mesh", "{\"maxTriangles\":100000}") + "," +
		Rule("resource.skeletal-mesh", "{\"maxVertices\":20000}") + "," +
		Rule("resource.sound", "{\"maxDurationMs\":60000,\"allowedFormats\":[\"OGG\"]}") + "]}";
	const auto config = cookscope::ParseRuleConfig(configJson);
	if (!config.ok)
	{
		return Fail("resource rule configuration must parse");
	}
	const cookscope::AnalysisResult result = cookscope::Evaluate(snapshot, config.value);
	if (!result.diagnostics.empty() || result.findings.size() != 8)
	{
		return Fail("resource rules must emit each measured violation without diagnostics");
	}

	std::map<std::string, int, std::less<>> counts;
	for (const cookscope::Finding& finding : result.findings) ++counts[finding.ruleId];
	if (counts["resource.texture"] != 4 || counts["resource.static-mesh"] != 1 ||
		counts["resource.skeletal-mesh"] != 1 || counts["resource.sound"] != 2)
	{
		return Fail("each resource rule must retain all violated metrics");
	}
	const auto width = std::find_if(result.findings.begin(), result.findings.end(), [](const cookscope::Finding& finding) {
		return finding.ruleId == "resource.texture" && finding.metric == "texture-width";
	});
	if (width == result.findings.end() || width->observedValue != std::optional<std::uint64_t>(4096) ||
		width->limitValue != std::optional<std::uint64_t>(2048) || width->measurementKind != cookscope::MeasurementKind::Unavailable)
	{
		return Fail("numeric metadata finding must retain metric/value/limit without a false size kind");
	}
	const auto format = std::find_if(result.findings.begin(), result.findings.end(), [](const cookscope::Finding& finding) {
		return finding.ruleId == "resource.sound" && finding.metric == "sound-format";
	});
	if (format == result.findings.end() || format->observedText != "PCM" || format->expectedText != "OGG")
	{
		return Fail("text metadata finding must retain observed and expected formats");
	}

	std::cout << "PASS: texture, mesh, and sound metadata budget contract\n";
	return 0;
}
