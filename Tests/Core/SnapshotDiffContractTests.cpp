#include "cookscope/diff.h"

#include <algorithm>
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

	cookscope::AssetRecord Asset(std::string path, std::uint64_t cookedBytes)
	{
		cookscope::AssetRecord asset;
		asset.objectPath = std::move(path);
		asset.cookedSize = {cookscope::MeasurementKind::ActualCooked, cookedBytes};
		return asset;
	}

	cookscope::Snapshot Snapshot(std::vector<cookscope::AssetRecord> assets, std::string platform = "Windows")
	{
		cookscope::Snapshot snapshot;
		snapshot.provenance = {
			"5.8.0-55116800",
			std::move(platform),
			"Development",
			"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
		snapshot.assets = std::move(assets);
		return snapshot;
	}
}

int main()
{
	using cookscope::AssetChangeKind;
	using cookscope::DependencyKind;
	using cookscope::EdgeChangeKind;

	cookscope::AssetRecord baselineA = Asset("/Game/A.A", 100);
	baselineA.primaryAssetId = "Type:A";
	baselineA.chunkIds = {0};
	baselineA.assetBundles = {"Default"};
	baselineA.dependencies = {{"/Game/B.B", DependencyKind::Soft}};
	cookscope::AssetRecord baselineRemoved = Asset("/Game/Removed.Removed", 20);
	cookscope::AssetRecord baselineRenamed = Asset("/Game/Old.Old", 10);
	baselineRenamed.tags = {{"StableAssetId", "rename-fixture"}};

	cookscope::AssetRecord candidateA = Asset("/Game/A.A", 150);
	candidateA.primaryAssetId = "Type2:A";
	candidateA.chunkIds = {1};
	candidateA.assetBundles = {"UI"};
	candidateA.dependencies = {
		{"/Game/B.B", DependencyKind::Hard},
		{"/Game/C.C", DependencyKind::Soft}};
	cookscope::AssetRecord candidateAdded = Asset("/Game/C.C", 30);
	cookscope::AssetRecord candidateRenamed = Asset("/Game/New.New", 10);
	candidateRenamed.tags = {{"StableAssetId", "rename-fixture"}};

	const cookscope::Snapshot baseline = Snapshot({baselineRenamed, baselineRemoved, baselineA});
	const cookscope::Snapshot candidate = Snapshot({candidateRenamed, candidateAdded, candidateA});
	const cookscope::SnapshotDiffResult result = cookscope::DiffSnapshots(baseline, candidate);
	if (!result.comparable || !result.error.empty())
	{
		return Fail("compatible snapshots must diff");
	}
	const auto added = std::find_if(result.assetChanges.begin(), result.assetChanges.end(), [](const cookscope::AssetChange& change) {
		return change.kind == AssetChangeKind::Added && change.candidatePath == "/Game/C.C";
	});
	const auto removed = std::find_if(result.assetChanges.begin(), result.assetChanges.end(), [](const cookscope::AssetChange& change) {
		return change.kind == AssetChangeKind::Removed && change.baselinePath == "/Game/Removed.Removed";
	});
	const auto modified = std::find_if(result.assetChanges.begin(), result.assetChanges.end(), [](const cookscope::AssetChange& change) {
		return change.kind == AssetChangeKind::Modified && change.candidatePath == "/Game/A.A";
	});
	const auto renamed = std::find_if(result.assetChanges.begin(), result.assetChanges.end(), [](const cookscope::AssetChange& change) {
		return change.kind == AssetChangeKind::Renamed && change.baselinePath == "/Game/Old.Old" && change.candidatePath == "/Game/New.New";
	});
	if (added == result.assetChanges.end() || removed == result.assetChanges.end() ||
		modified == result.assetChanges.end() || renamed == result.assetChanges.end())
	{
		return Fail("added, removed, modified, and StableAssetId rename changes are required");
	}
	for (const std::string_view field : {"assetBundles", "chunkIds", "cookedSize", "dependencies", "primaryAssetId"})
	{
		if (std::find(modified->fields.begin(), modified->fields.end(), field) == modified->fields.end())
		{
			return Fail("modified asset must list every changed semantic field");
		}
	}

	const auto typeChange = std::find_if(result.edgeChanges.begin(), result.edgeChanges.end(), [](const cookscope::EdgeChange& change) {
		return change.kind == EdgeChangeKind::TypeChanged && change.source == "/Game/A.A" && change.target == "/Game/B.B";
	});
	const auto edgeAdded = std::find_if(result.edgeChanges.begin(), result.edgeChanges.end(), [](const cookscope::EdgeChange& change) {
		return change.kind == EdgeChangeKind::Added && change.target == "/Game/C.C";
	});
	if (typeChange == result.edgeChanges.end() || typeChange->beforeKind != DependencyKind::Soft ||
		typeChange->afterKind != DependencyKind::Hard || edgeAdded == result.edgeChanges.end())
	{
		return Fail("edge additions and typed changes must remain explicit");
	}
	if (result.sizeChanges.size() != 1 || result.sizeChanges[0].assetPath != "/Game/A.A" || result.sizeChanges[0].deltaBytes != 50)
	{
		return Fail("actual cooked size delta must be exact and signed");
	}

	cookscope::Snapshot reorderedBaseline = baseline;
	std::reverse(reorderedBaseline.assets.begin(), reorderedBaseline.assets.end());
	cookscope::Snapshot reorderedCandidate = candidate;
	std::reverse(reorderedCandidate.assets.begin(), reorderedCandidate.assets.end());
	const cookscope::SnapshotDiffResult reordered = cookscope::DiffSnapshots(reorderedBaseline, reorderedCandidate);
	if (!reordered.comparable || cookscope::WriteCanonicalDiff(result) != cookscope::WriteCanonicalDiff(reordered))
	{
		return Fail("snapshot input order must not change canonical diff bytes");
	}

	const cookscope::SnapshotDiffResult incompatible = cookscope::DiffSnapshots(baseline, Snapshot({candidateA}, "Linux"));
	if (incompatible.comparable || incompatible.error != "snapshot platform mismatch")
	{
		return Fail("incompatible platform must fail closed with a stable reason");
	}

	std::cout << "PASS: deterministic compatible snapshot diff contract\n";
	return 0;
}
