#include "cookscope/diff.h"

#include "cookscope/json.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <tuple>
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
			using KindSet = std::set<DependencyKind>;
			std::map<std::string, KindSet, std::less<>> before;
			std::map<std::string, KindSet, std::less<>> after;
			for (const DependencyEdge& edge : baseline.dependencies) before[edge.target].insert(edge.kind);
			for (const DependencyEdge& edge : candidate.dependencies) after[edge.target].insert(edge.kind);
			std::set<std::string, std::less<>> targets;
			for (const auto& [target, ignored] : before) { (void)ignored; targets.insert(target); }
			for (const auto& [target, ignored] : after) { (void)ignored; targets.insert(target); }
			for (const std::string& target : targets)
			{
				KindSet oldKinds = before.contains(target) ? before.at(target) : KindSet{};
				KindSet newKinds = after.contains(target) ? after.at(target) : KindSet{};
				for (auto iterator = oldKinds.begin(); iterator != oldKinds.end();)
				{
					if (newKinds.erase(*iterator) != 0) iterator = oldKinds.erase(iterator);
					else ++iterator;
				}
				auto oldKind = oldKinds.begin();
				auto newKind = newKinds.begin();
				while (oldKind != oldKinds.end() && newKind != newKinds.end())
				{
					output.push_back({EdgeChangeKind::TypeChanged, candidate.objectPath, target, *oldKind, *newKind});
					++oldKind;
					++newKind;
				}
				for (; oldKind != oldKinds.end(); ++oldKind)
					output.push_back({EdgeChangeKind::Removed, candidate.objectPath, target, *oldKind, std::nullopt});
				for (; newKind != newKinds.end(); ++newKind)
					output.push_back({EdgeChangeKind::Added, candidate.objectPath, target, std::nullopt, *newKind});
			}
		}

		std::optional<std::int64_t> SignedDelta(std::uint64_t before, std::uint64_t after)
		{
			const std::uint64_t magnitude = before > after ? before - after : after - before;
			if (magnitude > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) return std::nullopt;
			return before > after ? -static_cast<std::int64_t>(magnitude) : static_cast<std::int64_t>(magnitude);
		}

		void AppendSizeChange(
			const AssetRecord& baseline,
			const AssetRecord& candidate,
			const std::string& resultPath,
			std::vector<CookedSizeChange>& output)
		{
			if (baseline.cookedSize.kind != MeasurementKind::ActualCooked ||
				candidate.cookedSize.kind != MeasurementKind::ActualCooked ||
				!baseline.cookedSize.bytes || !candidate.cookedSize.bytes ||
				baseline.cookedSize.bytes == candidate.cookedSize.bytes)
			{
				return;
			}
			const auto delta = SignedDelta(*baseline.cookedSize.bytes, *candidate.cookedSize.bytes);
			if (delta) output.push_back({resultPath, *baseline.cookedSize.bytes, *candidate.cookedSize.bytes, *delta});
		}

		std::string DiffAssetChangeName(AssetChangeKind kind)
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

		std::string DiffEdgeChangeName(EdgeChangeKind kind)
		{
			switch (kind)
			{
			case EdgeChangeKind::Added: return "added";
			case EdgeChangeKind::Removed: return "removed";
			case EdgeChangeKind::TypeChanged: return "type-changed";
			}
			return "added";
		}

		std::string DiffDependencyName(DependencyKind kind)
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

		std::string DiffSeverityName(Severity severity)
		{
			switch (severity)
			{
			case Severity::Note: return "note";
			case Severity::Warning: return "warning";
			case Severity::Error: return "error";
			}
			return "error";
		}

		std::string DiffFindingChangeName(FindingChangeKind kind)
		{
			switch (kind)
			{
			case FindingChangeKind::Added: return "added";
			case FindingChangeKind::Resolved: return "resolved";
			case FindingChangeKind::SeverityChanged: return "severity-changed";
			}
			return "added";
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
			AppendSizeChange(*asset, *same->second, path, result.sizeChanges);
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
			const AssetRecord& oldAsset = *before.at(oldPath);
			const AssetRecord& newAsset = *after.at(renamed->second);
			result.assetChanges.push_back({
				AssetChangeKind::Renamed,
				oldPath,
				renamed->second,
				ChangedFields(oldAsset, newAsset)});
			DiffEdges(oldAsset, newAsset, result.edgeChanges);
			AppendSizeChange(oldAsset, newAsset, renamed->second, result.sizeChanges);
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
			item.object.emplace("kind", DiffJsonString(DiffAssetChangeName(change.kind)));
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
			item.object.emplace("kind", DiffJsonString(DiffEdgeChangeName(change.kind)));
			item.object.emplace("source", DiffJsonString(change.source));
			item.object.emplace("target", DiffJsonString(change.target));
			item.object.emplace("beforeKind", change.beforeKind ? DiffJsonString(DiffDependencyName(*change.beforeKind)) : JsonValue{});
			item.object.emplace("afterKind", change.afterKind ? DiffJsonString(DiffDependencyName(*change.afterKind)) : JsonValue{});
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

		JsonValue findings;
		findings.type = JsonType::Array;
		for (const FindingChange& change : diff.findingChanges)
		{
			JsonValue item;
			item.type = JsonType::Object;
			item.object.emplace("kind", DiffJsonString(DiffFindingChangeName(change.kind)));
			item.object.emplace("ruleId", DiffJsonString(change.ruleId));
			item.object.emplace("assetPath", DiffJsonString(change.assetPath));
			item.object.emplace("beforeSeverity", change.beforeSeverity ? DiffJsonString(DiffSeverityName(*change.beforeSeverity)) : JsonValue{});
			item.object.emplace("afterSeverity", change.afterSeverity ? DiffJsonString(DiffSeverityName(*change.afterSeverity)) : JsonValue{});
			findings.array.push_back(std::move(item));
		}
		root.object.emplace("findingChanges", std::move(findings));
		return WriteCanonicalJson(root) + "\n";
	}

	void AppendFindingChanges(
		const AnalysisResult& baseline,
		const AnalysisResult& candidate,
		SnapshotDiffResult& diff)
	{
		using FindingKey = std::tuple<std::string, std::string, std::string, std::string>;
		std::map<FindingKey, Severity> before;
		std::map<FindingKey, Severity> after;
		auto key = [](const Finding& finding) {
			return FindingKey{finding.ruleId, finding.assetPath, finding.relatedAsset, finding.message};
		};
		for (const Finding& finding : baseline.findings) before.emplace(key(finding), finding.severity);
		for (const Finding& finding : candidate.findings) after.emplace(key(finding), finding.severity);
		std::set<FindingKey> keys;
		for (const auto& [item, ignored] : before) { (void)ignored; keys.insert(item); }
		for (const auto& [item, ignored] : after) { (void)ignored; keys.insert(item); }
		for (const FindingKey& item : keys)
		{
			const auto oldFinding = before.find(item);
			const auto newFinding = after.find(item);
			const auto& [ruleId, assetPath, relatedAsset, message] = item;
			(void)relatedAsset;
			(void)message;
			if (oldFinding == before.end())
			{
				diff.findingChanges.push_back({FindingChangeKind::Added, ruleId, assetPath, std::nullopt, newFinding->second});
			}
			else if (newFinding == after.end())
			{
				diff.findingChanges.push_back({FindingChangeKind::Resolved, ruleId, assetPath, oldFinding->second, std::nullopt});
			}
			else if (oldFinding->second != newFinding->second)
			{
				diff.findingChanges.push_back({FindingChangeKind::SeverityChanged, ruleId, assetPath, oldFinding->second, newFinding->second});
			}
		}
		std::sort(diff.findingChanges.begin(), diff.findingChanges.end(), [](const FindingChange& left, const FindingChange& right) {
			return left.ruleId < right.ruleId || (left.ruleId == right.ruleId &&
				(left.assetPath < right.assetPath || (left.assetPath == right.assetPath && left.kind < right.kind)));
		});
	}
}
