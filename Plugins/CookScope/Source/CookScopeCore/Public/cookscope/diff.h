#pragma once

#include "cookscope/snapshot.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cookscope
{
	enum class AssetChangeKind : std::uint8_t
	{
		Added,
		Removed,
		Modified,
		Renamed,
	};

	struct AssetChange
	{
		AssetChangeKind kind = AssetChangeKind::Modified;
		std::string baselinePath;
		std::string candidatePath;
		std::vector<std::string> fields;
	};

	enum class EdgeChangeKind : std::uint8_t
	{
		Added,
		Removed,
		TypeChanged,
	};

	struct EdgeChange
	{
		EdgeChangeKind kind = EdgeChangeKind::Added;
		std::string source;
		std::string target;
		std::optional<DependencyKind> beforeKind;
		std::optional<DependencyKind> afterKind;
	};

	struct CookedSizeChange
	{
		std::string assetPath;
		std::uint64_t beforeBytes = 0;
		std::uint64_t afterBytes = 0;
		std::int64_t deltaBytes = 0;
	};

	struct SnapshotDiffResult
	{
		bool comparable = false;
		std::string error;
		std::vector<AssetChange> assetChanges;
		std::vector<EdgeChange> edgeChanges;
		std::vector<CookedSizeChange> sizeChanges;
	};

	[[nodiscard]] COOKSCOPECORE_API SnapshotDiffResult DiffSnapshots(
		const Snapshot& baseline,
		const Snapshot& candidate);
	[[nodiscard]] COOKSCOPECORE_API std::string WriteCanonicalDiff(const SnapshotDiffResult& diff);
}

