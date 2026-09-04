#include "cookscope/diff.h"

#include "cookscope/json.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <utility>

namespace cookscope
{
	namespace
	{
		using AssetMap = std::map<std::string, const AssetRecord*, std::less<>>;

		AssetMap MapAssets(const Snapshot& snapshot)
		{
			AssetMap result;
			for (const AssetRecord& asset : snapshot.assets) result.emplace(asset.objectPath, &asset);
			return result;
		}

		std::string StableId(const AssetRecord& asset)
		{
			const auto found = asset.tags.find("StableAssetId");
			return found == asset.tags.end() ? std::string{} : found->second;
		}

		bool MeasurementEqual(const SizeMeasurement& left, const SizeMeasurement& right)
		{
			return left.kind == right.kind && left.bytes == right.bytes;
		}

		std::vector<DependencyEdge> SortedEdges(const AssetRecord& asset)
		{
			std::vector<DependencyEdge> result = asset.dependencies;
			std::sort(result.begin(), result.end(), [](const DependencyEdge& left, const DependencyEdge& right) {
				return left.target < right.target || (left.target == right.target && left.kind < right.kind);
			});
			return result;
		}

		bool EdgesEqual(const AssetRecord& left, const AssetRecord& right)
		{
			const auto leftEdges = SortedEdges(left);
			const auto rightEdges = SortedEdges(right);
			if (leftEdges.size() != rightEdges.size()) return false;
			for (std::size_t index = 0; index < leftEdges.size(); ++index)
			{
				if (leftEdges[index].target != rightEdges[index].target || leftEdges[index].kind != rightEdges[index].kind) return false;
			}
			return true;
		}

		std::vector<std::string> ChangedFields(const AssetRecord& baseline, const AssetRecord& candidate)
		{
			std::vector<std::string> fields;
			if (baseline.assetClass != candidate.assetClass) fields.push_back("assetClass");
			if (baseline.assetBundles != candidate.assetBundles) fields.push_back("assetBundles");
			if (baseline.chunkIds != candidate.chunkIds) fields.push_back("chunkIds");
			if (!MeasurementEqual(baseline.cookedSize, candidate.cookedSize)) fields.push_back("cookedSize");
			if (!EdgesEqual(baseline, candidate)) fields.push_back("dependencies");
			if (!MeasurementEqual(baseline.diskSize, candidate.diskSize)) fields.push_back("diskSize");
			if (baseline.packageName != candidate.packageName) fields.push_back("packageName");
			if (baseline.packagePath != candidate.packagePath) fields.push_back("packagePath");
			if (baseline.primaryAssetId != candidate.primaryAssetId) fields.push_back("primaryAssetId");
			if (baseline.tags != candidate.tags) fields.push_back("tags");
			std::sort(fields.begin(), fields.end());
			return fields;
		}

		void DiffEdges(
			const AssetRecord& baseline,
			const AssetRecord& candidate,
			std::vector<EdgeChange>& output)
		{
			std::map<std::string, DependencyKind, std::less<>> before;
			std::map<std::string, DependencyKind, std::less<>> after;
			for (const DependencyEdge& edge : baseline.dependencies) before.emplace(edge.target, edge.kind);
			for (const DependencyEdge& edge : candidate.dependencies) after.emplace(edge.target, edge.kind);
			std::set<std::string, std::less<>> targets;
			for (const auto& [target, ignored] : before) { (void)ignored; targets.insert(target); }
			for (const auto& [target, ignored] : after) { (void)ignored; targets.insert(target); }
			for (const std::string& target : targets)
			{
				const auto oldEdge = before.find(target);
				const auto newEdge = after.find(target);
				if (oldEdge == before.end())
				{
					output.push_back({EdgeChangeKind::Added, candidate.objectPath, target, std::nullopt, newEdge->second});
				}
				else if (newEdge == after.end())
				{
					output.push_back({EdgeChangeKind::Removed, candidate.objectPath, target, oldEdge->second, std::nullopt});
				}
				else if (oldEdge->second != newEdge->second)
				{
					output.push_back({EdgeChangeKind::TypeChanged, candidate.objectPath, target, oldEdge->second, newEdge->second});
				}
			}
		}

		std::optional<std::int64_t> SignedDelta(std::uint64_t before, std::uint64_t after)
		{
			const std::uint64_t magnitude = before > after ? before - after : after - before;
			if (magnitude > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) return std::nullopt;
			return before > after ? -static_cast<std::int64_t>(magnitude) : static_cast<std::int64_t>(magnitude);
		}

		std::string AssetChangeName(AssetChangeKind kind)
		{
			switch (kind)
			{
			case AssetChangeKind::Added: return "added";
			case AssetChangeKind::Removed: return "removed";
			case AssetChangeKind::Modified: return "modified";
			case AssetChangeKind::Renamed: return "renamed";
			}
			return "modified";
		}

		std::string EdgeChangeName(EdgeChangeKind kind)
		{
			switch (kind)
			{
			case EdgeChangeKind::Added: return "added";
			case EdgeChangeKind::Removed: return "removed";
			case EdgeChangeKind::TypeChanged: return "type-changed";
			}
			return "added";
		}

		std::string DependencyName(DependencyKind kind)
		{
			switch (kind)
			{
			case DependencyKind::Hard: return "hard";
			case DependencyKind::Soft: return "soft";
			case DependencyKind::Manage: return "manage";
			case DependencyKind::SearchableName: return "searchable-name";
			}
			return "hard";
		}

		JsonValue DiffJsonString(std::string value)
		{
			JsonValue result;
			result.type = JsonType::String;
			result.scalar = std::move(value);
			return result;
		}

		JsonValue DiffJsonNumber(std::string value)
		{
			JsonValue result;
			result.type = JsonType::Number;
			result.scalar = std::move(value);
			return result;
		}
	}

	SnapshotDiffResult DiffSnapshots(const Snapshot& baseline, const Snapshot& candidate)
	{
		SnapshotDiffResult result;
		if (baseline.schema != candidate.schema)
		{
			result.error = "snapshot schema mismatch";
			return result;
		}
		if (baseline.provenance.engineVersion != candidate.provenance.engineVersion)
		{
			result.error = "snapshot engine version mismatch";
			return result;
		}
		if (baseline.provenance.platform != candidate.provenance.platform)
		{
			result.error = "snapshot platform mismatch";
			return result;
		}
		if (baseline.provenance.cookConfiguration != candidate.provenance.cookConfiguration)
		{
			result.error = "snapshot Cook configuration mismatch";
			return result;
		}
		result.comparable = true;

		const AssetMap before = MapAssets(baseline);
		const AssetMap after = MapAssets(candidate);
		std::set<std::string, std::less<>> matchedBefore;
		std::set<std::string, std::less<>> matchedAfter;
		for (const auto& [path, asset] : before)
		{
			const auto same = after.find(path);
			if (same == after.end()) continue;
			matchedBefore.insert(path);
			matchedAfter.insert(path);
			std::vector<std::string> fields = ChangedFields(*asset, *same->second);
			if (!fields.empty()) result.assetChanges.push_back({AssetChangeKind::Modified, path, path, std::move(fields)});
			DiffEdges(*asset, *same->second, result.edgeChanges);
			if (asset->cookedSize.kind == MeasurementKind::ActualCooked && same->second->cookedSize.kind == MeasurementKind::ActualCooked &&
				asset->cookedSize.bytes && same->second->cookedSize.bytes && asset->cookedSize.bytes != same->second->cookedSize.bytes)
			{
				const auto delta = SignedDelta(*asset->cookedSize.bytes, *same->second->cookedSize.bytes);
				if (delta) result.sizeChanges.push_back({path, *asset->cookedSize.bytes, *same->second->cookedSize.bytes, *delta});
			}
		}

		std::map<std::string, std::string, std::less<>> beforeByStableId;
		std::map<std::string, std::string, std::less<>> afterByStableId;
		for (const auto& [path, asset] : before)
		{
			const std::string id = StableId(*asset);
			if (!id.empty() && !matchedBefore.contains(path)) beforeByStableId.emplace(id, path);
		}
		for (const auto& [path, asset] : after)
		{
			const std::string id = StableId(*asset);
			if (!id.empty() && !matchedAfter.contains(path)) afterByStableId.emplace(id, path);
		}
		for (const auto& [id, oldPath] : beforeByStableId)
		{
			const auto renamed = afterByStableId.find(id);
			if (renamed == afterByStableId.end()) continue;
			matchedBefore.insert(oldPath);
			matchedAfter.insert(renamed->second);
			result.assetChanges.push_back({AssetChangeKind::Renamed, oldPath, renamed->second, {}});
		}

		for (const auto& [path, ignored] : before)
		{
			(void)ignored;
			if (!matchedBefore.contains(path)) result.assetChanges.push_back({AssetChangeKind::Removed, path, {}, {}});
		}
		for (const auto& [path, ignored] : after)
		{
			(void)ignored;
			if (!matchedAfter.contains(path)) result.assetChanges.push_back({AssetChangeKind::Added, {}, path, {}});
		}

		std::sort(result.assetChanges.begin(), result.assetChanges.end(), [](const AssetChange& left, const AssetChange& right) {
			const std::string& leftPath = left.candidatePath.empty() ? left.baselinePath : left.candidatePath;
			const std::string& rightPath = right.candidatePath.empty() ? right.baselinePath : right.candidatePath;
			return leftPath < rightPath || (leftPath == rightPath && left.kind < right.kind);
		});
		std::sort(result.edgeChanges.begin(), result.edgeChanges.end(), [](const EdgeChange& left, const EdgeChange& right) {
			return left.source < right.source || (left.source == right.source &&
				(left.target < right.target || (left.target == right.target && left.kind < right.kind)));
		});
		std::sort(result.sizeChanges.begin(), result.sizeChanges.end(), [](const CookedSizeChange& left, const CookedSizeChange& right) {
			return left.assetPath < right.assetPath;
		});
		return result;
	}

	std::string WriteCanonicalDiff(const SnapshotDiffResult& diff)
	{
		JsonValue root;
		root.type = JsonType::Object;
		root.object.emplace("schema", DiffJsonString("cookscope.diff/1"));
		JsonValue comparable;
		comparable.type = JsonType::Boolean;
		comparable.boolean = diff.comparable;
		root.object.emplace("comparable", std::move(comparable));
		root.object.emplace("error", DiffJsonString(diff.error));

		JsonValue assets;
		assets.type = JsonType::Array;
		for (const AssetChange& change : diff.assetChanges)
		{
			JsonValue item;
			item.type = JsonType::Object;
			item.object.emplace("kind", DiffJsonString(AssetChangeName(change.kind)));
			item.object.emplace("baselinePath", DiffJsonString(change.baselinePath));
			item.object.emplace("candidatePath", DiffJsonString(change.candidatePath));
			JsonValue fields;
			fields.type = JsonType::Array;
			for (const std::string& field : change.fields) fields.array.push_back(DiffJsonString(field));
			item.object.emplace("fields", std::move(fields));
			assets.array.push_back(std::move(item));
		}
		root.object.emplace("assetChanges", std::move(assets));

		JsonValue edges;
		edges.type = JsonType::Array;
		for (const EdgeChange& change : diff.edgeChanges)
		{
			JsonValue item;
			item.type = JsonType::Object;
			item.object.emplace("kind", DiffJsonString(EdgeChangeName(change.kind)));
			item.object.emplace("source", DiffJsonString(change.source));
			item.object.emplace("target", DiffJsonString(change.target));
			item.object.emplace("beforeKind", change.beforeKind ? DiffJsonString(DependencyName(*change.beforeKind)) : JsonValue{});
			item.object.emplace("afterKind", change.afterKind ? DiffJsonString(DependencyName(*change.afterKind)) : JsonValue{});
			edges.array.push_back(std::move(item));
		}
		root.object.emplace("edgeChanges", std::move(edges));

		JsonValue sizes;
		sizes.type = JsonType::Array;
		for (const CookedSizeChange& change : diff.sizeChanges)
		{
			JsonValue item;
			item.type = JsonType::Object;
			item.object.emplace("assetPath", DiffJsonString(change.assetPath));
			item.object.emplace("beforeBytes", DiffJsonNumber(std::to_string(change.beforeBytes)));
			item.object.emplace("afterBytes", DiffJsonNumber(std::to_string(change.afterBytes)));
			item.object.emplace("deltaBytes", DiffJsonNumber(std::to_string(change.deltaBytes)));
			sizes.array.push_back(std::move(item));
		}
		root.object.emplace("sizeChanges", std::move(sizes));
		return WriteCanonicalJson(root) + "\n";
	}
}
