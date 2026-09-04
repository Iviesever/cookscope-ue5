#include "cookscope/snapshot.h"

#include <algorithm>
#include <charconv>
#include <initializer_list>
#include <limits>
#include <set>
#include <utility>

namespace cookscope
{
	namespace
	{
		class SnapshotReader
		{
		public:
			SnapshotParseResult Read(std::string_view json)
			{
				SnapshotParseResult result;
				JsonParseResult syntax = ParseJson(json);
				if (!syntax.ok)
				{
					result.error = {SnapshotErrorCode::JsonSyntax, syntax.error.path, syntax.error.message};
					return result;
				}
				if (syntax.value.type != JsonType::Object)
				{
					result.error = {SnapshotErrorCode::ExpectedObject, "$", "snapshot root must be an object"};
					return result;
				}
				if (!CheckFields(syntax.value, {"assets", "provenance", "schema"}, "$"))
				{
					result.error = std::move(Error);
					return result;
				}

				const JsonValue* schema = Require(syntax.value, "schema", "$", JsonType::String);
				const JsonValue* provenance = Require(syntax.value, "provenance", "$", JsonType::Object);
				const JsonValue* assets = Require(syntax.value, "assets", "$", JsonType::Array);
				if (!schema || !provenance || !assets)
				{
					result.error = std::move(Error);
					return result;
				}
				if (schema->scalar != "cookscope.snapshot/1")
				{
					Fail(SnapshotErrorCode::InvalidValue, "$.schema", "unsupported snapshot schema");
					result.error = std::move(Error);
					return result;
				}

				Snapshot snapshot;
				if (!ReadProvenance(*provenance, "$.provenance", snapshot.provenance))
				{
					result.error = std::move(Error);
					return result;
				}
				std::set<std::string, std::less<>> objectPaths;
				for (std::size_t index = 0; index < assets->array.size(); ++index)
				{
					AssetRecord asset;
					const std::string path = "$.assets[" + std::to_string(index) + "]";
					if (!ReadAsset(assets->array[index], path, asset))
					{
						result.error = std::move(Error);
						return result;
					}
					if (!objectPaths.insert(asset.objectPath).second)
					{
						Fail(SnapshotErrorCode::DuplicateAsset, path + ".objectPath", "duplicate asset object path");
						result.error = std::move(Error);
						return result;
					}
					snapshot.assets.push_back(std::move(asset));
				}
				std::sort(snapshot.assets.begin(), snapshot.assets.end(), [](const AssetRecord& left, const AssetRecord& right) {
					return left.objectPath < right.objectPath;
				});

				result.ok = true;
				result.value = std::move(snapshot);
				return result;
			}

		private:
			bool ReadProvenance(const JsonValue& value, const std::string& path, SnapshotProvenance& output)
			{
				if (!CheckFields(value, {"cookConfiguration", "engineVersion", "platform", "sourceSha"}, path))
				{
					return false;
				}
				const JsonValue* engine = Require(value, "engineVersion", path, JsonType::String);
				const JsonValue* platform = Require(value, "platform", path, JsonType::String);
				const JsonValue* configuration = Require(value, "cookConfiguration", path, JsonType::String);
				const JsonValue* sha = Require(value, "sourceSha", path, JsonType::String);
				if (!engine || !platform || !configuration || !sha)
				{
					return false;
				}
				if (engine->scalar.empty() || platform->scalar.empty() || configuration->scalar.empty() || !IsSha(sha->scalar))
				{
					return Fail(SnapshotErrorCode::InvalidValue, path, "provenance fields must be non-empty and sourceSha must be 40 hexadecimal characters");
				}
				output = {engine->scalar, platform->scalar, configuration->scalar, sha->scalar};
				std::transform(output.sourceSha.begin(), output.sourceSha.end(), output.sourceSha.begin(), [](unsigned char value) {
					return static_cast<char>(value >= 'A' && value <= 'F' ? value - 'A' + 'a' : value);
				});
				return true;
			}

			bool ReadAsset(const JsonValue& value, const std::string& path, AssetRecord& output)
			{
				if (value.type != JsonType::Object)
				{
					return Fail(SnapshotErrorCode::ExpectedObject, path, "asset must be an object");
				}
				if (!CheckFields(value, {
					"assetBundles", "assetClass", "chunkIds", "cookedSize", "dependencies", "diskSize",
					"objectPath", "packageName", "packagePath", "primaryAssetId", "sourceProvenance", "tags"
				}, path))
				{
					return false;
				}

				const JsonValue* objectPath = Require(value, "objectPath", path, JsonType::String);
				const JsonValue* packageName = Require(value, "packageName", path, JsonType::String);
				const JsonValue* assetClass = Require(value, "assetClass", path, JsonType::String);
				const JsonValue* packagePath = Require(value, "packagePath", path, JsonType::String);
				const JsonValue* primaryAssetId = Find(value, "primaryAssetId", path);
				const JsonValue* diskSize = Require(value, "diskSize", path, JsonType::Object);
				const JsonValue* cookedSize = Require(value, "cookedSize", path, JsonType::Object);
				const JsonValue* chunkIds = Require(value, "chunkIds", path, JsonType::Array);
				const JsonValue* bundles = Require(value, "assetBundles", path, JsonType::Array);
				const JsonValue* tags = Require(value, "tags", path, JsonType::Object);
				const JsonValue* dependencies = Require(value, "dependencies", path, JsonType::Array);
				const JsonValue* source = Require(value, "sourceProvenance", path, JsonType::String);
				if (!objectPath || !packageName || !assetClass || !packagePath || !primaryAssetId || !diskSize || !cookedSize ||
					!chunkIds || !bundles || !tags || !dependencies || !source)
				{
					return false;
				}
				if (objectPath->scalar.empty() || packageName->scalar.empty() || assetClass->scalar.empty() ||
					packagePath->scalar.empty() || source->scalar.empty())
				{
					return Fail(SnapshotErrorCode::InvalidValue, path, "asset identity and provenance fields must not be empty");
				}
				output.objectPath = objectPath->scalar;
				output.packageName = packageName->scalar;
				output.assetClass = assetClass->scalar;
				output.packagePath = packagePath->scalar;
				output.sourceProvenance = source->scalar;

				if (primaryAssetId->type == JsonType::String)
				{
					if (primaryAssetId->scalar.empty())
					{
						return Fail(SnapshotErrorCode::InvalidValue, path + ".primaryAssetId", "Primary Asset ID must be null or non-empty");
					}
					output.primaryAssetId = primaryAssetId->scalar;
				}
				else if (primaryAssetId->type != JsonType::Null)
				{
					return Fail(SnapshotErrorCode::InvalidType, path + ".primaryAssetId", "Primary Asset ID must be a string or null");
				}

				return ReadMeasurement(*diskSize, path + ".diskSize", output.diskSize) &&
					ReadMeasurement(*cookedSize, path + ".cookedSize", output.cookedSize) &&
					ReadChunkIds(*chunkIds, path + ".chunkIds", output.chunkIds) &&
					ReadStringArray(*bundles, path + ".assetBundles", output.assetBundles) &&
					ReadTags(*tags, path + ".tags", output.tags) &&
					ReadDependencies(*dependencies, path + ".dependencies", output.dependencies);
			}

			bool ReadMeasurement(const JsonValue& value, const std::string& path, SizeMeasurement& output)
			{
				if (!CheckFields(value, {"bytes", "kind"}, path))
				{
					return false;
				}
				const JsonValue* kind = Require(value, "kind", path, JsonType::String);
				if (!kind || !ReadMeasurementKind(kind->scalar, path + ".kind", output.kind))
				{
					return false;
				}
				const auto bytes = value.object.find("bytes");
				if (output.kind == MeasurementKind::Unavailable)
				{
					if (bytes != value.object.end())
					{
						return Fail(SnapshotErrorCode::InvalidValue, path + ".bytes", "unavailable measurement must not contain bytes");
					}
					return true;
				}
				if (bytes == value.object.end())
				{
					return Fail(SnapshotErrorCode::MissingField, path + ".bytes", "available measurement requires bytes");
				}
				std::uint64_t parsed = 0;
				if (!ReadUnsigned(bytes->second, path + ".bytes", parsed))
				{
					return false;
				}
				output.bytes = parsed;
				return true;
			}

			bool ReadChunkIds(const JsonValue& value, const std::string& path, std::vector<std::int32_t>& output)
			{
				std::set<std::int32_t> seen;
				for (std::size_t index = 0; index < value.array.size(); ++index)
				{
					std::uint64_t parsed = 0;
					const std::string itemPath = path + "[" + std::to_string(index) + "]";
					if (!ReadUnsigned(value.array[index], itemPath, parsed) || parsed > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
					{
						return Fail(SnapshotErrorCode::InvalidValue, itemPath, "Chunk ID must be a unique non-negative 32-bit integer");
					}
					const std::int32_t chunk = static_cast<std::int32_t>(parsed);
					if (!seen.insert(chunk).second)
					{
						return Fail(SnapshotErrorCode::InvalidValue, itemPath, "duplicate Chunk ID");
					}
					output.push_back(chunk);
				}
				std::sort(output.begin(), output.end());
				output.erase(std::unique(output.begin(), output.end()), output.end());
				return true;
			}

			bool ReadStringArray(const JsonValue& value, const std::string& path, std::vector<std::string>& output)
			{
				for (std::size_t index = 0; index < value.array.size(); ++index)
				{
					if (value.array[index].type != JsonType::String || value.array[index].scalar.empty())
					{
						return Fail(SnapshotErrorCode::InvalidType, path + "[" + std::to_string(index) + "]", "value must be a non-empty string");
					}
					output.push_back(value.array[index].scalar);
				}
				std::sort(output.begin(), output.end());
				output.erase(std::unique(output.begin(), output.end()), output.end());
				return true;
			}

			bool ReadTags(const JsonValue& value, const std::string& path, std::map<std::string, std::string, std::less<>>& output)
			{
				for (const auto& [key, item] : value.object)
				{
					if (item.type != JsonType::String)
					{
						return Fail(SnapshotErrorCode::InvalidType, path + "." + key, "tag value must be a string");
					}
					output.emplace(key, item.scalar);
				}
				return true;
			}

			bool ReadDependencies(const JsonValue& value, const std::string& path, std::vector<DependencyEdge>& output)
			{
				for (std::size_t index = 0; index < value.array.size(); ++index)
				{
					const JsonValue& item = value.array[index];
					const std::string itemPath = path + "[" + std::to_string(index) + "]";
					if (item.type != JsonType::Object)
					{
						return Fail(SnapshotErrorCode::ExpectedObject, itemPath, "dependency must be an object");
					}
					if (!CheckFields(item, {"kind", "target"}, itemPath))
					{
						return false;
					}
					const JsonValue* target = Require(item, "target", itemPath, JsonType::String);
					const JsonValue* kind = Require(item, "kind", itemPath, JsonType::String);
					DependencyKind parsedKind = DependencyKind::Hard;
					if (!target || !kind || target->scalar.empty() || !ReadDependencyKind(kind->scalar, itemPath + ".kind", parsedKind))
					{
						if (target && target->scalar.empty()) Fail(SnapshotErrorCode::InvalidValue, itemPath + ".target", "dependency target must not be empty");
						return false;
					}
					output.push_back({target->scalar, parsedKind});
				}
				std::sort(output.begin(), output.end(), [](const DependencyEdge& left, const DependencyEdge& right) {
					return left.kind < right.kind || (left.kind == right.kind && left.target < right.target);
				});
				output.erase(
					std::unique(output.begin(), output.end(), [](const DependencyEdge& left, const DependencyEdge& right) {
						return left.kind == right.kind && left.target == right.target;
					}),
					output.end());
				return true;
			}

			bool ReadUnsigned(const JsonValue& value, const std::string& path, std::uint64_t& output)
			{
				if (value.type != JsonType::Number)
				{
					return Fail(SnapshotErrorCode::InvalidType, path, "value must be an unsigned integer");
				}
				const auto parsed = std::from_chars(value.scalar.data(), value.scalar.data() + value.scalar.size(), output);
				if (parsed.ec != std::errc{} || parsed.ptr != value.scalar.data() + value.scalar.size())
				{
					return Fail(SnapshotErrorCode::InvalidValue, path, "value must be an unsigned 64-bit integer");
				}
				return true;
			}

			const JsonValue* Find(const JsonValue& object, std::string_view name, const std::string& path)
			{
				const auto found = object.object.find(name);
				if (found == object.object.end())
				{
					Fail(SnapshotErrorCode::MissingField, path + "." + std::string(name), "missing required field");
					return nullptr;
				}
				return &found->second;
			}

			const JsonValue* Require(const JsonValue& object, std::string_view name, const std::string& path, JsonType type)
			{
				const JsonValue* found = Find(object, name, path);
				if (found && found->type != type)
				{
					Fail(SnapshotErrorCode::InvalidType, path + "." + std::string(name), "field has the wrong JSON type");
					return nullptr;
				}
				return found;
			}

			bool CheckFields(const JsonValue& object, std::initializer_list<std::string_view> allowed, const std::string& path)
			{
				for (const auto& [name, ignored] : object.object)
				{
					(void)ignored;
					if (std::find(allowed.begin(), allowed.end(), name) == allowed.end())
					{
						return Fail(SnapshotErrorCode::UnknownField, path + "." + name, "unknown field");
					}
				}
				return true;
			}

			bool ReadMeasurementKind(std::string_view value, const std::string& path, MeasurementKind& output)
			{
				if (value == "source-disk") output = MeasurementKind::SourceDisk;
				else if (value == "package-disk") output = MeasurementKind::PackageDisk;
				else if (value == "estimated") output = MeasurementKind::Estimated;
				else if (value == "actual-cooked") output = MeasurementKind::ActualCooked;
				else if (value == "unavailable") output = MeasurementKind::Unavailable;
				else return Fail(SnapshotErrorCode::InvalidValue, path, "unknown measurement kind");
				return true;
			}

			bool ReadDependencyKind(std::string_view value, const std::string& path, DependencyKind& output)
			{
				if (value == "hard") output = DependencyKind::Hard;
				else if (value == "soft") output = DependencyKind::Soft;
				else if (value == "manage") output = DependencyKind::Manage;
				else if (value == "searchable-name") output = DependencyKind::SearchableName;
				else return Fail(SnapshotErrorCode::InvalidValue, path, "unknown dependency kind");
				return true;
			}

			bool Fail(SnapshotErrorCode code, std::string path, std::string message)
			{
				if (Error.code == SnapshotErrorCode::None)
				{
					Error = {code, std::move(path), std::move(message)};
				}
				return false;
			}

			static bool IsSha(std::string_view value)
			{
				if (value.size() != 40) return false;
				return std::all_of(value.begin(), value.end(), [](unsigned char character) {
					return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f') ||
						(character >= 'A' && character <= 'F');
				});
			}

			SnapshotError Error;
		};

		JsonValue SnapshotJsonString(std::string value)
		{
			JsonValue result;
			result.type = JsonType::String;
			result.scalar = std::move(value);
			return result;
		}

		JsonValue SnapshotJsonNumber(std::uint64_t value)
		{
			JsonValue result;
			result.type = JsonType::Number;
			result.scalar = std::to_string(value);
			return result;
		}

		std::string_view SnapshotMeasurementName(MeasurementKind kind)
		{
			switch (kind)
			{
			case MeasurementKind::SourceDisk: return "source-disk";
			case MeasurementKind::PackageDisk: return "package-disk";
			case MeasurementKind::Estimated: return "estimated";
			case MeasurementKind::ActualCooked: return "actual-cooked";
			case MeasurementKind::Unavailable: return "unavailable";
			}
			return "unavailable";
		}

		std::string_view SnapshotDependencyName(DependencyKind kind)
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

		JsonValue Measurement(const SizeMeasurement& measurement)
		{
			JsonValue result;
			result.type = JsonType::Object;
			result.object.emplace("kind", SnapshotJsonString(std::string(SnapshotMeasurementName(measurement.kind))));
			if (measurement.bytes.has_value()) result.object.emplace("bytes", SnapshotJsonNumber(*measurement.bytes));
			return result;
		}
	}

	SnapshotParseResult ParseSnapshot(std::string_view utf8Json)
	{
		return SnapshotReader().Read(utf8Json);
	}

	std::string WriteCanonicalSnapshot(const Snapshot& snapshot)
	{
		JsonValue root;
		root.type = JsonType::Object;
		root.object.emplace("schema", SnapshotJsonString(snapshot.schema));

		JsonValue provenance;
		provenance.type = JsonType::Object;
		provenance.object.emplace("engineVersion", SnapshotJsonString(snapshot.provenance.engineVersion));
		provenance.object.emplace("platform", SnapshotJsonString(snapshot.provenance.platform));
		provenance.object.emplace("cookConfiguration", SnapshotJsonString(snapshot.provenance.cookConfiguration));
		provenance.object.emplace("sourceSha", SnapshotJsonString(snapshot.provenance.sourceSha));
		root.object.emplace("provenance", std::move(provenance));

		std::vector<AssetRecord> assets = snapshot.assets;
		std::sort(assets.begin(), assets.end(), [](const AssetRecord& left, const AssetRecord& right) {
			return left.objectPath < right.objectPath;
		});
		JsonValue assetArray;
		assetArray.type = JsonType::Array;
		for (AssetRecord& asset : assets)
		{
			JsonValue value;
			value.type = JsonType::Object;
			value.object.emplace("objectPath", SnapshotJsonString(asset.objectPath));
			value.object.emplace("packageName", SnapshotJsonString(asset.packageName));
			value.object.emplace("assetClass", SnapshotJsonString(asset.assetClass));
			value.object.emplace("packagePath", SnapshotJsonString(asset.packagePath));
			value.object.emplace("primaryAssetId", asset.primaryAssetId ? SnapshotJsonString(*asset.primaryAssetId) : JsonValue{});
			value.object.emplace("diskSize", Measurement(asset.diskSize));
			value.object.emplace("cookedSize", Measurement(asset.cookedSize));

			std::sort(asset.chunkIds.begin(), asset.chunkIds.end());
			JsonValue chunks;
			chunks.type = JsonType::Array;
			for (const std::int32_t chunk : asset.chunkIds) chunks.array.push_back(SnapshotJsonNumber(static_cast<std::uint64_t>(chunk)));
			value.object.emplace("chunkIds", std::move(chunks));

			std::sort(asset.assetBundles.begin(), asset.assetBundles.end());
			asset.assetBundles.erase(std::unique(asset.assetBundles.begin(), asset.assetBundles.end()), asset.assetBundles.end());
			JsonValue bundles;
			bundles.type = JsonType::Array;
			for (const std::string& bundle : asset.assetBundles) bundles.array.push_back(SnapshotJsonString(bundle));
			value.object.emplace("assetBundles", std::move(bundles));

			JsonValue tags;
			tags.type = JsonType::Object;
			for (const auto& [key, tagValue] : asset.tags) tags.object.emplace(key, SnapshotJsonString(tagValue));
			value.object.emplace("tags", std::move(tags));

			std::sort(asset.dependencies.begin(), asset.dependencies.end(), [](const DependencyEdge& left, const DependencyEdge& right) {
				return left.kind < right.kind || (left.kind == right.kind && left.target < right.target);
			});
			asset.dependencies.erase(
				std::unique(asset.dependencies.begin(), asset.dependencies.end(), [](const DependencyEdge& left, const DependencyEdge& right) {
					return left.kind == right.kind && left.target == right.target;
				}),
				asset.dependencies.end());
			JsonValue dependencies;
			dependencies.type = JsonType::Array;
			for (const DependencyEdge& edge : asset.dependencies)
			{
				JsonValue dependency;
				dependency.type = JsonType::Object;
				dependency.object.emplace("target", SnapshotJsonString(edge.target));
				dependency.object.emplace("kind", SnapshotJsonString(std::string(SnapshotDependencyName(edge.kind))));
				dependencies.array.push_back(std::move(dependency));
			}
			value.object.emplace("dependencies", std::move(dependencies));
			value.object.emplace("sourceProvenance", SnapshotJsonString(asset.sourceProvenance));
			assetArray.array.push_back(std::move(value));
		}
		root.object.emplace("assets", std::move(assetArray));
		return WriteCanonicalJson(root) + "\n";
	}
}
