#pragma once

#include "cookscope/json.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cookscope
{
	enum class DependencyKind : std::uint8_t
	{
		Hard,
		Soft,
		Manage,
		SearchableName,
	};

	enum class MeasurementKind : std::uint8_t
	{
		SourceDisk,
		PackageDisk,
		Estimated,
		ActualCooked,
		Unavailable,
	};

	struct SizeMeasurement
	{
		MeasurementKind kind = MeasurementKind::Unavailable;
		std::optional<std::uint64_t> bytes;
	};

	struct DependencyEdge
	{
		std::string target;
		DependencyKind kind = DependencyKind::Hard;
	};

	struct AssetRecord
	{
		std::string objectPath;
		std::string packageName;
		std::string assetClass;
		std::string packagePath;
		std::optional<std::string> primaryAssetId;
		SizeMeasurement diskSize;
		SizeMeasurement cookedSize;
		std::vector<std::int32_t> chunkIds;
		std::vector<std::string> assetBundles;
		std::map<std::string, std::string, std::less<>> tags;
		std::vector<DependencyEdge> dependencies;
		std::string sourceProvenance;
	};

	struct SnapshotProvenance
	{
		std::string engineVersion;
		std::string platform;
		std::string cookConfiguration;
		std::string sourceSha;
	};

	struct Snapshot
	{
		std::string schema = "cookscope.snapshot/1";
		SnapshotProvenance provenance;
		std::vector<AssetRecord> assets;
	};

	enum class SnapshotErrorCode : std::uint8_t
	{
		None,
		JsonSyntax,
		ExpectedObject,
		MissingField,
		UnknownField,
		InvalidType,
		InvalidValue,
		DuplicateAsset,
	};

	struct SnapshotError
	{
		SnapshotErrorCode code = SnapshotErrorCode::None;
		std::string path = "$";
		std::string message;
	};

	struct SnapshotParseResult
	{
		bool ok = false;
		Snapshot value;
		SnapshotError error;
	};

	[[nodiscard]] COOKSCOPECORE_API SnapshotParseResult ParseSnapshot(std::string_view utf8Json);
	[[nodiscard]] COOKSCOPECORE_API std::string WriteCanonicalSnapshot(const Snapshot& snapshot);
}

