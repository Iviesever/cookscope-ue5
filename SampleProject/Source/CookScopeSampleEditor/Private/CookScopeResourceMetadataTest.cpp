#include "CookScopeAssetScanner.h"

#include "cookscope/rule_config.h"
#include "cookscope/rules.h"

#include "Misc/AutomationTest.h"

#include <algorithm>
#include <string>

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCookScopeResourceMetadataTest,
	"CookScope.PACT30.ResourceMetadataAdapter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	const cookscope::AssetRecord* FindResource(
		const cookscope::Snapshot& Snapshot,
		const std::string& AssetClass)
	{
		const auto Found = std::find_if(Snapshot.assets.begin(), Snapshot.assets.end(), [&](const cookscope::AssetRecord& Asset) {
			return Asset.assetClass == AssetClass;
		});
		return Found == Snapshot.assets.end() ? nullptr : &*Found;
	}

	bool HasPositiveIntegerTag(const cookscope::AssetRecord* Asset, const char* Name)
	{
		if (!Asset) return false;
		const auto Found = Asset->tags.find(Name);
		if (Found == Asset->tags.end()) return false;
		return !Found->second.empty() && Found->second != "0";
	}

	std::string TagValue(const cookscope::AssetRecord* Asset, const char* Name)
	{
		if (!Asset) return {};
		const auto Found = Asset->tags.find(Name);
		return Found == Asset->tags.end() ? std::string{} : Found->second;
	}
}

bool FCookScopeResourceMetadataTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FCookScopeScanResult Scan = FCookScopeAssetScanner::ScanPath(
		TEXT("/Game/CookScopeResourceFixtures"),
		TEXT("0000000000000000000000000000000000000000"));
	TestTrue(TEXT("Real resource fixture scan succeeds"), Scan.bSuccess);
	if (!Scan.bSuccess) return false;
	TestEqual(TEXT("Four real resource assets are scanned"), static_cast<int32>(Scan.Snapshot.assets.size()), 4);

	const cookscope::AssetRecord* Texture = FindResource(Scan.Snapshot, "/Script/Engine.Texture2D");
	const cookscope::AssetRecord* StaticMesh = FindResource(Scan.Snapshot, "/Script/Engine.StaticMesh");
	const cookscope::AssetRecord* SkeletalMesh = FindResource(Scan.Snapshot, "/Script/Engine.SkeletalMesh");
	const cookscope::AssetRecord* Sound = FindResource(Scan.Snapshot, "/Script/Engine.SoundWave");
	TestTrue(TEXT("Texture exposes real normalized dimensions"),
		HasPositiveIntegerTag(Texture, "TextureWidth") && HasPositiveIntegerTag(Texture, "TextureHeight") &&
		HasPositiveIntegerTag(Texture, "TextureMips") && Texture->tags.contains("TextureFormat"));
	TestTrue(TEXT("Static Mesh exposes real triangle count"), HasPositiveIntegerTag(StaticMesh, "MeshTriangles"));
	TestTrue(TEXT("Skeletal Mesh exposes real vertex count"), HasPositiveIntegerTag(SkeletalMesh, "MeshVertices"));
	TestTrue(TEXT("Sound exposes real duration and format"),
		HasPositiveIntegerTag(Sound, "SoundDurationMs") && Sound->tags.contains("SoundFormat"));

	const cookscope::RuleConfigParseResult Config = cookscope::ParseRuleConfig(
		R"({"schema":"cookscope.rules/1","rules":[{"id":"resource.skeletal-mesh","name":"Skeletal Mesh","description":"Real adapter fixture","severity":"warning","scope":{"include":["/Game/**"],"exclude":[]},"parameters":{"maxVertices":0},"exceptions":[],"baseline":"report-all","failThreshold":"error","helpUri":"docs/rules/resource.md"},{"id":"resource.sound","name":"Sound","description":"Real adapter fixture","severity":"warning","scope":{"include":["/Game/**"],"exclude":[]},"parameters":{"maxDurationMs":0,"allowedFormats":["NeverFormat"]},"exceptions":[],"baseline":"report-all","failThreshold":"error","helpUri":"docs/rules/resource.md"},{"id":"resource.static-mesh","name":"Static Mesh","description":"Real adapter fixture","severity":"warning","scope":{"include":["/Game/**"],"exclude":[]},"parameters":{"maxTriangles":0},"exceptions":[],"baseline":"report-all","failThreshold":"error","helpUri":"docs/rules/resource.md"},{"id":"resource.texture","name":"Texture","description":"Real adapter fixture","severity":"warning","scope":{"include":["/Game/**"],"exclude":[]},"parameters":{"maxWidth":0,"maxHeight":0,"minMips":999,"allowedFormats":["NeverFormat"]},"exceptions":[],"baseline":"report-all","failThreshold":"error","helpUri":"docs/rules/resource.md"}]})");
	TestTrue(TEXT("Real resource rule config parses"), Config.ok);
	if (!Config.ok) return false;
	const cookscope::AnalysisResult Analysis = cookscope::Evaluate(Scan.Snapshot, Config.value);
	TestEqual(TEXT("Real UE metadata produces all eight resource findings"), static_cast<int32>(Analysis.findings.size()), 8);
	TestEqual(TEXT("Real UE resource metadata has no unavailable diagnostics"), static_cast<int32>(Analysis.diagnostics.size()), 0);

	const std::string PositiveJson =
		std::string("{\"schema\":\"cookscope.rules/1\",\"rules\":[") +
		"{\"id\":\"resource.skeletal-mesh\",\"name\":\"Skeletal Mesh\",\"description\":\"Real legal fixture\",\"severity\":\"warning\",\"scope\":{\"include\":[\"/Game/**\"],\"exclude\":[]},\"parameters\":{\"maxVertices\":" + TagValue(SkeletalMesh, "MeshVertices") + "},\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\",\"helpUri\":\"docs/rules/resource.md\"}," +
		"{\"id\":\"resource.sound\",\"name\":\"Sound\",\"description\":\"Real legal fixture\",\"severity\":\"warning\",\"scope\":{\"include\":[\"/Game/**\"],\"exclude\":[]},\"parameters\":{\"maxDurationMs\":" + TagValue(Sound, "SoundDurationMs") + ",\"allowedFormats\":[\"" + TagValue(Sound, "SoundFormat") + "\"]},\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\",\"helpUri\":\"docs/rules/resource.md\"}," +
		"{\"id\":\"resource.static-mesh\",\"name\":\"Static Mesh\",\"description\":\"Real legal fixture\",\"severity\":\"warning\",\"scope\":{\"include\":[\"/Game/**\"],\"exclude\":[]},\"parameters\":{\"maxTriangles\":" + TagValue(StaticMesh, "MeshTriangles") + "},\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\",\"helpUri\":\"docs/rules/resource.md\"}," +
		"{\"id\":\"resource.texture\",\"name\":\"Texture\",\"description\":\"Real legal fixture\",\"severity\":\"warning\",\"scope\":{\"include\":[\"/Game/**\"],\"exclude\":[]},\"parameters\":{\"maxWidth\":" + TagValue(Texture, "TextureWidth") + ",\"maxHeight\":" + TagValue(Texture, "TextureHeight") + ",\"minMips\":" + TagValue(Texture, "TextureMips") + ",\"allowedFormats\":[\"" + TagValue(Texture, "TextureFormat") + "\"]},\"exceptions\":[],\"baseline\":\"report-all\",\"failThreshold\":\"error\",\"helpUri\":\"docs/rules/resource.md\"}]}";
	const cookscope::RuleConfigParseResult PositiveConfig = cookscope::ParseRuleConfig(PositiveJson);
	TestTrue(TEXT("Real legal resource config parses"), PositiveConfig.ok);
	if (!PositiveConfig.ok) return false;
	const cookscope::AnalysisResult Positive = cookscope::Evaluate(Scan.Snapshot, PositiveConfig.value);
	TestEqual(TEXT("Real legal resource thresholds produce no findings"), static_cast<int32>(Positive.findings.size()), 0);
	TestEqual(TEXT("Real legal resource thresholds produce no diagnostics"), static_cast<int32>(Positive.diagnostics.size()), 0);
	return true;
}

#endif
