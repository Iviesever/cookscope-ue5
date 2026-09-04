#include "cookscope/rules.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <utility>

namespace cookscope
{
	namespace
	{
		bool MatchesScope(const RuleDefinition& rule, std::string_view assetPath)
		{
			const bool included = std::any_of(rule.scope.include.begin(), rule.scope.include.end(), [&](const std::string& selector) {
				return GlobMatches(selector, assetPath);
			});
			if (!included) return false;
			const bool excluded = std::any_of(rule.scope.exclude.begin(), rule.scope.exclude.end(), [&](const std::string& selector) {
				return GlobMatches(selector, assetPath);
			});
			if (excluded) return false;
			return std::none_of(rule.exceptions.begin(), rule.exceptions.end(), [&](const RuleException& exception) {
				return GlobMatches(exception.selector, assetPath);
			});
		}

		std::string_view AssetName(std::string_view objectPath)
		{
			const std::size_t slash = objectPath.find_last_of('/');
			const std::size_t start = slash == std::string_view::npos ? 0 : slash + 1;
			const std::size_t dot = objectPath.find('.', start);
			return objectPath.substr(start, dot == std::string_view::npos ? objectPath.size() - start : dot - start);
		}

		const JsonValue* Parameter(const RuleDefinition& rule, std::string_view name)
		{
			const auto found = rule.parameters.object.find(name);
			return found == rule.parameters.object.end() ? nullptr : &found->second;
		}

		void AddParameterDiagnostic(AnalysisResult& result, const RuleDefinition& rule, std::string message)
		{
			result.diagnostics.push_back({
				rule.id,
				{},
				AnalysisDiagnosticCode::InvalidRuleParameters,
				std::move(message)});
		}

		void EvaluatePrefix(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			const JsonValue* prefixes = Parameter(rule, "classPrefixes");
			if (!prefixes || prefixes->type != JsonType::Object)
			{
				AddParameterDiagnostic(result, rule, "classPrefixes must be an object of class path to prefix");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath)) continue;
				const auto prefix = prefixes->object.find(asset.assetClass);
				if (prefix == prefixes->object.end()) continue;
				if (prefix->second.type != JsonType::String || prefix->second.scalar.empty())
				{
					AddParameterDiagnostic(result, rule, "class prefix values must be non-empty strings");
					return;
				}
				if (!AssetName(asset.objectPath).starts_with(prefix->second.scalar))
				{
					result.findings.push_back({
						rule.id,
						asset.objectPath,
						rule.severity,
						"asset name must start with '" + prefix->second.scalar + "'",
					});
				}
			}
		}

		void EvaluateForbiddenPath(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			const JsonValue* patterns = Parameter(rule, "patterns");
			if (!patterns || patterns->type != JsonType::Array)
			{
				AddParameterDiagnostic(result, rule, "patterns must be an array of path globs");
				return;
			}
			for (const JsonValue& pattern : patterns->array)
			{
				if (pattern.type != JsonType::String || pattern.scalar.empty())
				{
					AddParameterDiagnostic(result, rule, "path patterns must be non-empty strings");
					return;
				}
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath)) continue;
				for (const JsonValue& pattern : patterns->array)
				{
					if (GlobMatches(pattern.scalar, asset.objectPath))
					{
						result.findings.push_back({
							rule.id,
							asset.objectPath,
							rule.severity,
							"asset is under forbidden path '" + pattern.scalar + "'",
						});
						break;
					}
				}
			}
		}

		bool ReadBudget(const RuleDefinition& rule, std::uint64_t& output)
		{
			const JsonValue* budget = Parameter(rule, "budgetBytes");
			if (!budget || budget->type != JsonType::Number) return false;
			const auto parsed = std::from_chars(budget->scalar.data(), budget->scalar.data() + budget->scalar.size(), output);
			return parsed.ec == std::errc{} && parsed.ptr == budget->scalar.data() + budget->scalar.size();
		}

		const SizeMeasurement* SelectMeasurement(const AssetRecord& asset, std::string_view requested)
		{
			if (requested == "source-disk" && asset.diskSize.kind == MeasurementKind::SourceDisk) return &asset.diskSize;
			if (requested == "package-disk" && asset.diskSize.kind == MeasurementKind::PackageDisk) return &asset.diskSize;
			if (requested == "estimated")
			{
				if (asset.diskSize.kind == MeasurementKind::Estimated) return &asset.diskSize;
				if (asset.cookedSize.kind == MeasurementKind::Estimated) return &asset.cookedSize;
			}
			if (requested == "actual-cooked" && asset.cookedSize.kind == MeasurementKind::ActualCooked) return &asset.cookedSize;
			return nullptr;
		}

		MeasurementKind RequestedKind(std::string_view requested)
		{
			if (requested == "source-disk") return MeasurementKind::SourceDisk;
			if (requested == "package-disk") return MeasurementKind::PackageDisk;
			if (requested == "estimated") return MeasurementKind::Estimated;
			if (requested == "actual-cooked") return MeasurementKind::ActualCooked;
			return MeasurementKind::Unavailable;
		}

		void EvaluateBudget(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::uint64_t budget = 0;
			const JsonValue* measurement = Parameter(rule, "measurement");
			if (!ReadBudget(rule, budget) || !measurement || measurement->type != JsonType::String ||
				RequestedKind(measurement->scalar) == MeasurementKind::Unavailable)
			{
				AddParameterDiagnostic(result, rule, "budget rule requires unsigned budgetBytes and a known measurement");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath)) continue;
				const SizeMeasurement* measured = SelectMeasurement(asset, measurement->scalar);
				if (!measured || !measured->bytes.has_value())
				{
					result.diagnostics.push_back({
						rule.id,
						asset.objectPath,
						AnalysisDiagnosticCode::MeasurementUnavailable,
						"requested measurement is unavailable"});
					continue;
				}
				if (*measured->bytes > budget)
				{
					result.findings.push_back({
						rule.id,
						asset.objectPath,
						rule.severity,
						"asset size exceeds budget",
						measured->kind,
						measured->bytes,
						budget});
				}
			}
		}

		bool ReadUnsignedParameter(const RuleDefinition& rule, std::string_view name, std::uint64_t& output)
		{
			const JsonValue* value = Parameter(rule, name);
			if (!value || value->type != JsonType::Number) return false;
			const auto parsed = std::from_chars(value->scalar.data(), value->scalar.data() + value->scalar.size(), output);
			return parsed.ec == std::errc{} && parsed.ptr == value->scalar.data() + value->scalar.size();
		}

		bool ReadStringArrayParameter(const RuleDefinition& rule, std::string_view name, std::vector<std::string>& output)
		{
			const JsonValue* value = Parameter(rule, name);
			if (!value || value->type != JsonType::Array) return false;
			for (const JsonValue& item : value->array)
			{
				if (item.type != JsonType::String || item.scalar.empty()) return false;
				output.push_back(item.scalar);
			}
			return !output.empty();
		}

		bool MatchesAny(const std::vector<std::string>& patterns, std::string_view value)
		{
			return std::any_of(patterns.begin(), patterns.end(), [&](const std::string& pattern) {
				return GlobMatches(pattern, value);
			});
		}

		bool ParseDependencyKind(std::string_view name, DependencyKind& output)
		{
			if (name == "hard") output = DependencyKind::Hard;
			else if (name == "soft") output = DependencyKind::Soft;
			else if (name == "manage") output = DependencyKind::Manage;
			else if (name == "searchable-name") output = DependencyKind::SearchableName;
			else return false;
			return true;
		}

		Finding DependencyFinding(
			const RuleDefinition& rule,
			const DependencyStep& edge,
			std::string message)
		{
			Finding finding;
			finding.ruleId = rule.id;
			finding.assetPath = edge.source;
			finding.severity = rule.severity;
			finding.message = std::move(message);
			finding.relatedAsset = edge.target;
			finding.dependencyKind = edge.kind;
			finding.dependencyPath.push_back(edge);
			return finding;
		}

		void EvaluateForbiddenDependency(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::vector<std::string> fromPatterns;
			std::vector<std::string> toPatterns;
			std::vector<std::string> kindNames;
			if (!ReadStringArrayParameter(rule, "from", fromPatterns) ||
				!ReadStringArrayParameter(rule, "to", toPatterns) ||
				!ReadStringArrayParameter(rule, "kinds", kindNames))
			{
				AddParameterDiagnostic(result, rule, "forbidden dependency requires non-empty from, to, and kinds arrays");
				return;
			}
			std::vector<DependencyKind> kinds;
			for (const std::string& name : kindNames)
			{
				DependencyKind kind = DependencyKind::Hard;
				if (!ParseDependencyKind(name, kind))
				{
					AddParameterDiagnostic(result, rule, "unknown dependency kind");
					return;
				}
				kinds.push_back(kind);
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath) || !MatchesAny(fromPatterns, asset.objectPath)) continue;
				for (const DependencyEdge& edge : asset.dependencies)
				{
					if (MatchesAny(toPatterns, edge.target) && std::find(kinds.begin(), kinds.end(), edge.kind) != kinds.end())
					{
						result.findings.push_back(DependencyFinding(
							rule,
							{asset.objectPath, edge.target, edge.kind},
							"forbidden dependency boundary crossed"));
					}
				}
			}
		}

		void EvaluateRuntimeToEditor(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::vector<std::string> runtimePatterns;
			std::vector<std::string> editorPatterns;
			if (!ReadStringArrayParameter(rule, "runtimePatterns", runtimePatterns) ||
				!ReadStringArrayParameter(rule, "editorPatterns", editorPatterns))
			{
				AddParameterDiagnostic(result, rule, "runtime-to-editor requires runtimePatterns and editorPatterns");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath) || !MatchesAny(runtimePatterns, asset.objectPath)) continue;
				for (const DependencyEdge& edge : asset.dependencies)
				{
					if (MatchesAny(editorPatterns, edge.target))
					{
						result.findings.push_back(DependencyFinding(
							rule,
							{asset.objectPath, edge.target, edge.kind},
							"runtime asset depends on editor-only content"));
					}
				}
			}
		}

		void EvaluateCycles(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			const GraphBuildResult graph = BuildDependencyGraph(snapshot, OperationLimits{});
			if (graph.state != OperationState::Complete)
			{
				AddParameterDiagnostic(result, rule, "dependency graph could not be built within limits");
				return;
			}
			const CyclesResult cycles = FindCycles(graph.graph, DependencyMask::All(), OperationLimits{});
			if (cycles.state != OperationState::Complete)
			{
				AddParameterDiagnostic(result, rule, "cycle detection was truncated");
				return;
			}
			for (const Cycle& cycle : cycles.cycles)
			{
				if (cycle.nodes.empty() || !MatchesScope(rule, cycle.nodes.front())) continue;
				Finding finding;
				finding.ruleId = rule.id;
				finding.assetPath = cycle.nodes.front();
				finding.relatedAsset = cycle.nodes.back();
				finding.severity = rule.severity;
				finding.message = "dependency cycle contains " + std::to_string(cycle.nodes.size()) + " assets";
				result.findings.push_back(std::move(finding));
			}
		}

		void EvaluateFanOut(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::uint64_t maximum = 0;
			if (!ReadUnsignedParameter(rule, "maxFanOut", maximum))
			{
				AddParameterDiagnostic(result, rule, "maxFanOut must be an unsigned integer");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath) || asset.dependencies.size() <= maximum) continue;
				Finding finding;
				finding.ruleId = rule.id;
				finding.assetPath = asset.objectPath;
				finding.severity = rule.severity;
				finding.message = "direct dependency fan-out exceeds " + std::to_string(maximum);
				result.findings.push_back(std::move(finding));
			}
		}

		std::vector<DependencyStep> FindDepthViolation(
			const DependencyGraph& graph,
			std::size_t source,
			std::size_t maximumDepth)
		{
			std::vector<bool> inPath(graph.nodes.size(), false);
			std::vector<DependencyStep> path;
			std::vector<DependencyStep> found;
			std::function<bool(std::size_t)> visit = [&](std::size_t node) {
				if (path.size() > maximumDepth)
				{
					found = path;
					return true;
				}
				inPath[node] = true;
				for (const GraphArc& edge : graph.outgoing[node])
				{
					if (inPath[edge.node]) continue;
					path.push_back({graph.nodes[node], graph.nodes[edge.node], edge.kind});
					if (visit(edge.node)) return true;
					path.pop_back();
				}
				inPath[node] = false;
				return false;
			};
			visit(source);
			return found;
		}

		void EvaluateDepth(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::uint64_t maximum = 0;
			if (!ReadUnsignedParameter(rule, "maxDepth", maximum) || maximum > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
			{
				AddParameterDiagnostic(result, rule, "maxDepth must fit in size_t");
				return;
			}
			const GraphBuildResult graph = BuildDependencyGraph(snapshot, OperationLimits{});
			if (graph.state != OperationState::Complete)
			{
				AddParameterDiagnostic(result, rule, "dependency graph could not be built within limits");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath)) continue;
				const auto node = graph.graph.nodeIndices.find(asset.objectPath);
				if (node == graph.graph.nodeIndices.end()) continue;
				std::vector<DependencyStep> path = FindDepthViolation(graph.graph, node->second, static_cast<std::size_t>(maximum));
				if (path.empty()) continue;
				Finding finding;
				finding.ruleId = rule.id;
				finding.assetPath = asset.objectPath;
				finding.relatedAsset = path.back().target;
				finding.dependencyKind = path.front().kind;
				finding.dependencyPath = std::move(path);
				finding.severity = rule.severity;
				finding.message = "dependency depth exceeds " + std::to_string(maximum);
				result.findings.push_back(std::move(finding));
			}
		}

		bool ReadTagUnsigned(const AssetRecord& asset, std::string_view tag, std::uint64_t& output)
		{
			const auto found = asset.tags.find(tag);
			if (found == asset.tags.end()) return false;
			const auto parsed = std::from_chars(found->second.data(), found->second.data() + found->second.size(), output);
			return parsed.ec == std::errc{} && parsed.ptr == found->second.data() + found->second.size();
		}

		std::vector<std::string> ReadAllowedFormats(const RuleDefinition& rule)
		{
			std::vector<std::string> formats;
			ReadStringArrayParameter(rule, "allowedFormats", formats);
			std::sort(formats.begin(), formats.end());
			return formats;
		}

		std::string Join(const std::vector<std::string>& values)
		{
			std::string result;
			for (std::size_t index = 0; index < values.size(); ++index)
			{
				if (index != 0) result += ',';
				result += values[index];
			}
			return result;
		}

		void AddNumericMetric(
			AnalysisResult& result,
			const RuleDefinition& rule,
			const AssetRecord& asset,
			std::string metric,
			std::uint64_t observed,
			std::uint64_t limit,
			std::string message)
		{
			Finding finding;
			finding.ruleId = rule.id;
			finding.assetPath = asset.objectPath;
			finding.severity = rule.severity;
			finding.message = std::move(message);
			finding.metric = std::move(metric);
			finding.observedValue = observed;
			finding.limitValue = limit;
			result.findings.push_back(std::move(finding));
		}

		void AddTextMetric(
			AnalysisResult& result,
			const RuleDefinition& rule,
			const AssetRecord& asset,
			std::string metric,
			std::string observed,
			std::string expected,
			std::string message)
		{
			Finding finding;
			finding.ruleId = rule.id;
			finding.assetPath = asset.objectPath;
			finding.severity = rule.severity;
			finding.message = std::move(message);
			finding.metric = std::move(metric);
			finding.observedText = std::move(observed);
			finding.expectedText = std::move(expected);
			result.findings.push_back(std::move(finding));
		}

		void EvaluateTexture(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::uint64_t maximumWidth = 0;
			std::uint64_t maximumHeight = 0;
			std::uint64_t minimumMips = 0;
			const std::vector<std::string> formats = ReadAllowedFormats(rule);
			if (!ReadUnsignedParameter(rule, "maxWidth", maximumWidth) ||
				!ReadUnsignedParameter(rule, "maxHeight", maximumHeight) ||
				!ReadUnsignedParameter(rule, "minMips", minimumMips) || formats.empty())
			{
				AddParameterDiagnostic(result, rule, "texture rule requires maxWidth, maxHeight, minMips, and allowedFormats");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (asset.assetClass != "/Script/Engine.Texture2D" || !MatchesScope(rule, asset.objectPath)) continue;
				std::uint64_t width = 0;
				std::uint64_t height = 0;
				std::uint64_t mips = 0;
				const auto format = asset.tags.find("TextureFormat");
				if (!ReadTagUnsigned(asset, "TextureWidth", width) || !ReadTagUnsigned(asset, "TextureHeight", height) ||
					!ReadTagUnsigned(asset, "TextureMips", mips) || format == asset.tags.end())
				{
					result.diagnostics.push_back({rule.id, asset.objectPath, AnalysisDiagnosticCode::MeasurementUnavailable, "texture metadata is unavailable"});
					continue;
				}
				if (width > maximumWidth) AddNumericMetric(result, rule, asset, "texture-width", width, maximumWidth, "texture width exceeds limit");
				if (height > maximumHeight) AddNumericMetric(result, rule, asset, "texture-height", height, maximumHeight, "texture height exceeds limit");
				if (mips < minimumMips) AddNumericMetric(result, rule, asset, "texture-mips", mips, minimumMips, "texture mip count is below minimum");
				if (std::find(formats.begin(), formats.end(), format->second) == formats.end())
				{
					AddTextMetric(result, rule, asset, "texture-format", format->second, Join(formats), "texture format is not allowed");
				}
			}
		}

		void EvaluateStaticMesh(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::uint64_t maximum = 0;
			if (!ReadUnsignedParameter(rule, "maxTriangles", maximum))
			{
				AddParameterDiagnostic(result, rule, "static mesh rule requires maxTriangles");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (asset.assetClass != "/Script/Engine.StaticMesh" || !MatchesScope(rule, asset.objectPath)) continue;
				std::uint64_t observed = 0;
				if (!ReadTagUnsigned(asset, "MeshTriangles", observed))
				{
					result.diagnostics.push_back({rule.id, asset.objectPath, AnalysisDiagnosticCode::MeasurementUnavailable, "static mesh triangle metadata is unavailable"});
				}
				else if (observed > maximum)
				{
					AddNumericMetric(result, rule, asset, "static-mesh-triangles", observed, maximum, "static mesh triangle count exceeds limit");
				}
			}
		}

		void EvaluateSkeletalMesh(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::uint64_t maximum = 0;
			if (!ReadUnsignedParameter(rule, "maxVertices", maximum))
			{
				AddParameterDiagnostic(result, rule, "skeletal mesh rule requires maxVertices");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (asset.assetClass != "/Script/Engine.SkeletalMesh" || !MatchesScope(rule, asset.objectPath)) continue;
				std::uint64_t observed = 0;
				if (!ReadTagUnsigned(asset, "MeshVertices", observed))
				{
					result.diagnostics.push_back({rule.id, asset.objectPath, AnalysisDiagnosticCode::MeasurementUnavailable, "skeletal mesh vertex metadata is unavailable"});
				}
				else if (observed > maximum)
				{
					AddNumericMetric(result, rule, asset, "skeletal-mesh-vertices", observed, maximum, "skeletal mesh vertex count exceeds limit");
				}
			}
		}

		void EvaluateSound(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::uint64_t maximumDuration = 0;
			const std::vector<std::string> formats = ReadAllowedFormats(rule);
			if (!ReadUnsignedParameter(rule, "maxDurationMs", maximumDuration) || formats.empty())
			{
				AddParameterDiagnostic(result, rule, "sound rule requires maxDurationMs and allowedFormats");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (asset.assetClass != "/Script/Engine.SoundWave" || !MatchesScope(rule, asset.objectPath)) continue;
				std::uint64_t duration = 0;
				const auto format = asset.tags.find("SoundFormat");
				if (!ReadTagUnsigned(asset, "SoundDurationMs", duration) || format == asset.tags.end())
				{
					result.diagnostics.push_back({rule.id, asset.objectPath, AnalysisDiagnosticCode::MeasurementUnavailable, "sound metadata is unavailable"});
					continue;
				}
				if (duration > maximumDuration) AddNumericMetric(result, rule, asset, "sound-duration-ms", duration, maximumDuration, "sound duration exceeds limit");
				if (std::find(formats.begin(), formats.end(), format->second) == formats.end())
				{
					AddTextMetric(result, rule, asset, "sound-format", format->second, Join(formats), "sound format is not allowed");
				}
			}
		}

		bool ReadStringParameter(const RuleDefinition& rule, std::string_view name, std::string& output)
		{
			const JsonValue* value = Parameter(rule, name);
			if (!value || value->type != JsonType::String || value->scalar.empty()) return false;
			output = value->scalar;
			return true;
		}

		Finding SimpleFinding(const RuleDefinition& rule, const AssetRecord& asset, std::string message)
		{
			Finding finding;
			finding.ruleId = rule.id;
			finding.assetPath = asset.objectPath;
			finding.severity = rule.severity;
			finding.message = std::move(message);
			return finding;
		}

		bool IsActuallyCooked(const AssetRecord& asset)
		{
			return asset.cookedSize.kind == MeasurementKind::ActualCooked && asset.cookedSize.bytes.has_value();
		}

		void EvaluatePrimaryRequired(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (MatchesScope(rule, asset.objectPath) && !asset.primaryAssetId.has_value())
				{
					result.findings.push_back(SimpleFinding(rule, asset, "asset is missing required Primary Asset configuration"));
				}
			}
		}

		void EvaluateBundleRequired(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::string required;
			if (!ReadStringParameter(rule, "bundle", required))
			{
				AddParameterDiagnostic(result, rule, "bundle-required needs a non-empty bundle name");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (MatchesScope(rule, asset.objectPath) &&
					std::find(asset.assetBundles.begin(), asset.assetBundles.end(), required) == asset.assetBundles.end())
				{
					Finding finding = SimpleFinding(rule, asset, "required Asset Bundle is missing");
					finding.expectedText = required;
					result.findings.push_back(std::move(finding));
				}
			}
		}

		void EvaluatePrimaryType(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::string expected;
			if (!ReadStringParameter(rule, "expectedType", expected))
			{
				AddParameterDiagnostic(result, rule, "primary-type needs expectedType");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath) || !asset.primaryAssetId) continue;
				const std::size_t colon = asset.primaryAssetId->find(':');
				const std::string actual = asset.primaryAssetId->substr(0, colon);
				if (actual != expected)
				{
					Finding finding = SimpleFinding(rule, asset, "Primary Asset type does not match expected type");
					finding.observedText = actual;
					finding.expectedText = expected;
					result.findings.push_back(std::move(finding));
				}
			}
		}

		void EvaluateChunkConflict(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::uint64_t maximum = 0;
			if (!ReadUnsignedParameter(rule, "maxChunks", maximum))
			{
				AddParameterDiagnostic(result, rule, "chunk-conflict needs maxChunks");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (MatchesScope(rule, asset.objectPath) && asset.chunkIds.size() > maximum)
				{
					Finding finding = SimpleFinding(rule, asset, "asset belongs to too many Chunks");
					finding.metric = "chunk-count";
					finding.observedValue = asset.chunkIds.size();
					finding.limitValue = maximum;
					result.findings.push_back(std::move(finding));
				}
			}
		}

		void EvaluateChunkRequired(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::uint64_t required = 0;
			if (!ReadUnsignedParameter(rule, "chunkId", required) || required > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()))
			{
				AddParameterDiagnostic(result, rule, "chunk-required needs a non-negative 32-bit chunkId");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath)) continue;
				const std::int32_t requiredChunk = static_cast<std::int32_t>(required);
				if (std::find(asset.chunkIds.begin(), asset.chunkIds.end(), requiredChunk) == asset.chunkIds.end())
				{
					Finding finding = SimpleFinding(rule, asset, "required Chunk assignment is missing");
					finding.metric = "required-chunk";
					finding.expectedText = std::to_string(required);
					result.findings.push_back(std::move(finding));
				}
			}
		}

		void EvaluateUnexpectedCook(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::vector<std::string> allowed;
			if (!ReadStringArrayParameter(rule, "allowedPatterns", allowed))
			{
				AddParameterDiagnostic(result, rule, "cook.unexpected needs allowedPatterns");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (MatchesScope(rule, asset.objectPath) && IsActuallyCooked(asset) && !MatchesAny(allowed, asset.objectPath))
				{
					Finding finding = SimpleFinding(rule, asset, "asset was cooked outside the allowed set");
					finding.measurementKind = MeasurementKind::ActualCooked;
					finding.observedBytes = asset.cookedSize.bytes;
					result.findings.push_back(std::move(finding));
				}
			}
		}

		void EvaluateMissingCook(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::vector<std::string> expected;
			if (!ReadStringArrayParameter(rule, "expectedPatterns", expected))
			{
				AddParameterDiagnostic(result, rule, "cook.missing needs expectedPatterns");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (MatchesScope(rule, asset.objectPath) && MatchesAny(expected, asset.objectPath) && !IsActuallyCooked(asset))
				{
					result.findings.push_back(SimpleFinding(rule, asset, "expected asset is absent from actual Cook data"));
				}
			}
		}

		void EvaluateEditorLeak(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::vector<std::string> editorPatterns;
			if (!ReadStringArrayParameter(rule, "editorPatterns", editorPatterns))
			{
				AddParameterDiagnostic(result, rule, "editor-only leak rule needs editorPatterns");
				return;
			}
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (MatchesScope(rule, asset.objectPath) && MatchesAny(editorPatterns, asset.objectPath) && IsActuallyCooked(asset))
				{
					Finding finding = SimpleFinding(rule, asset, "editor-only asset leaked into actual Cook output");
					finding.measurementKind = MeasurementKind::ActualCooked;
					finding.observedBytes = asset.cookedSize.bytes;
					result.findings.push_back(std::move(finding));
				}
			}
		}

		void EvaluateCookRuleConflict(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath)) continue;
				const auto always = asset.tags.find("AlwaysCook");
				const auto never = asset.tags.find("NeverCook");
				if (always != asset.tags.end() && never != asset.tags.end() && always->second == "true" && never->second == "true")
				{
					result.findings.push_back(SimpleFinding(rule, asset, "AlwaysCook and NeverCook are both active"));
				}
			}
		}

		void EvaluateRedirector(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (MatchesScope(rule, asset.objectPath) && asset.assetClass == "/Script/CoreUObject.ObjectRedirector")
				{
					result.findings.push_back(SimpleFinding(rule, asset, "asset is a redirector"));
				}
			}
		}

		void EvaluateMissingReferences(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::set<std::string, std::less<>> assets;
			for (const AssetRecord& asset : snapshot.assets) assets.insert(asset.objectPath);
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath)) continue;
				for (const DependencyEdge& edge : asset.dependencies)
				{
					if (assets.contains(edge.target)) continue;
					result.findings.push_back(DependencyFinding(
						rule,
						{asset.objectPath, edge.target, edge.kind},
						"dependency target is unresolved"));
				}
			}
		}

		void EvaluateAmbiguousNames(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::map<std::string, std::vector<std::string>, std::less<>> byName;
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (MatchesScope(rule, asset.objectPath)) byName[std::string(AssetName(asset.objectPath))].push_back(asset.objectPath);
			}
			for (auto& [name, paths] : byName)
			{
				(void)name;
				if (paths.size() < 2) continue;
				std::sort(paths.begin(), paths.end());
				Finding finding;
				finding.ruleId = rule.id;
				finding.assetPath = paths[0];
				finding.relatedAsset = paths[1];
				finding.severity = rule.severity;
				finding.message = "asset name is ambiguous across paths";
				result.findings.push_back(std::move(finding));
			}
		}

		template <typename Predicate>
		void EvaluateAggregateBudget(
			const Snapshot& snapshot,
			const RuleDefinition& rule,
			AnalysisResult& result,
			std::string findingPath,
			Predicate&& predicate)
		{
			std::uint64_t budget = 0;
			const JsonValue* measurement = Parameter(rule, "measurement");
			if (!ReadBudget(rule, budget) || !measurement || measurement->type != JsonType::String ||
				RequestedKind(measurement->scalar) == MeasurementKind::Unavailable)
			{
				AddParameterDiagnostic(result, rule, "aggregate budget requires unsigned budgetBytes and a known measurement");
				return;
			}

			std::uint64_t total = 0;
			bool complete = true;
			for (const AssetRecord& asset : snapshot.assets)
			{
				if (!MatchesScope(rule, asset.objectPath) || !predicate(asset)) continue;
				const SizeMeasurement* measured = SelectMeasurement(asset, measurement->scalar);
				if (!measured || !measured->bytes)
				{
					result.diagnostics.push_back({
						rule.id,
						asset.objectPath,
						AnalysisDiagnosticCode::MeasurementUnavailable,
						"aggregate measurement is unavailable"});
					complete = false;
					continue;
				}
				if (*measured->bytes > std::numeric_limits<std::uint64_t>::max() - total)
				{
					AddParameterDiagnostic(result, rule, "aggregate byte total overflowed uint64");
					complete = false;
					continue;
				}
				total += *measured->bytes;
			}
			if (complete && total > budget)
			{
				Finding finding;
				finding.ruleId = rule.id;
				finding.assetPath = std::move(findingPath);
				finding.severity = rule.severity;
				finding.message = "aggregate size exceeds budget";
				finding.measurementKind = RequestedKind(measurement->scalar);
				finding.observedBytes = total;
				finding.budgetBytes = budget;
				result.findings.push_back(std::move(finding));
			}
		}

		void EvaluateDirectoryBudget(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::vector<std::string> patterns;
			if (!ReadStringArrayParameter(rule, "patterns", patterns))
			{
				AddParameterDiagnostic(result, rule, "directory budget needs patterns");
				return;
			}
			EvaluateAggregateBudget(snapshot, rule, result, patterns.front(), [&](const AssetRecord& asset) {
				return MatchesAny(patterns, asset.objectPath);
			});
		}

		void EvaluateTypeBudget(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			std::string assetClass;
			if (!ReadStringParameter(rule, "assetClass", assetClass))
			{
				AddParameterDiagnostic(result, rule, "type budget needs assetClass");
				return;
			}
			EvaluateAggregateBudget(snapshot, rule, result, assetClass, [&](const AssetRecord& asset) {
				return asset.assetClass == assetClass;
			});
		}

		void EvaluateProjectBudget(const Snapshot& snapshot, const RuleDefinition& rule, AnalysisResult& result)
		{
			EvaluateAggregateBudget(snapshot, rule, result, "$project", [](const AssetRecord&) { return true; });
		}
	}

	bool GlobMatches(std::string_view pattern, std::string_view value)
	{
		const std::size_t width = value.size() + 1;
		std::vector<std::int8_t> memo((pattern.size() + 1) * width, -1);
		std::function<bool(std::size_t, std::size_t)> match = [&](std::size_t patternIndex, std::size_t valueIndex) {
			std::int8_t& cached = memo[patternIndex * width + valueIndex];
			if (cached != -1) return cached == 1;
			bool result = false;
			if (patternIndex == pattern.size())
			{
				result = valueIndex == value.size();
			}
			else if (pattern[patternIndex] == '*')
			{
				const bool recursive = patternIndex + 1 < pattern.size() && pattern[patternIndex + 1] == '*';
				const std::size_t nextPattern = patternIndex + (recursive ? 2 : 1);
				result = match(nextPattern, valueIndex);
				if (!result && valueIndex < value.size() && (recursive || value[valueIndex] != '/'))
				{
					result = match(patternIndex, valueIndex + 1);
				}
			}
			else if (valueIndex < value.size() && pattern[patternIndex] == '?')
			{
				result = value[valueIndex] != '/' && match(patternIndex + 1, valueIndex + 1);
			}
			else if (valueIndex < value.size() && pattern[patternIndex] == value[valueIndex])
			{
				result = match(patternIndex + 1, valueIndex + 1);
			}
			cached = result ? 1 : 0;
			return result;
		};
		return match(0, 0);
	}

	AnalysisResult Evaluate(const Snapshot& snapshot, const RuleConfig& config)
	{
		AnalysisResult result;
		for (const RuleDefinition& rule : config.rules)
		{
			if (rule.id == "naming.asset-prefix") EvaluatePrefix(snapshot, rule, result);
			else if (rule.id == "path.forbidden") EvaluateForbiddenPath(snapshot, rule, result);
			else if (rule.id == "dependency.forbidden") EvaluateForbiddenDependency(snapshot, rule, result);
			else if (rule.id == "dependency.runtime-to-editor") EvaluateRuntimeToEditor(snapshot, rule, result);
			else if (rule.id == "dependency.cycle") EvaluateCycles(snapshot, rule, result);
			else if (rule.id == "dependency.max-fanout") EvaluateFanOut(snapshot, rule, result);
			else if (rule.id == "dependency.max-depth") EvaluateDepth(snapshot, rule, result);
			else if (rule.id == "resource.texture") EvaluateTexture(snapshot, rule, result);
			else if (rule.id == "resource.static-mesh") EvaluateStaticMesh(snapshot, rule, result);
			else if (rule.id == "resource.skeletal-mesh") EvaluateSkeletalMesh(snapshot, rule, result);
			else if (rule.id == "resource.sound") EvaluateSound(snapshot, rule, result);
			else if (rule.id == "asset-manager.primary-required") EvaluatePrimaryRequired(snapshot, rule, result);
			else if (rule.id == "asset-manager.bundle-required") EvaluateBundleRequired(snapshot, rule, result);
			else if (rule.id == "asset-manager.primary-type") EvaluatePrimaryType(snapshot, rule, result);
			else if (rule.id == "asset-manager.chunk-conflict") EvaluateChunkConflict(snapshot, rule, result);
			else if (rule.id == "asset-manager.chunk-required") EvaluateChunkRequired(snapshot, rule, result);
			else if (rule.id == "cook.unexpected") EvaluateUnexpectedCook(snapshot, rule, result);
			else if (rule.id == "cook.missing") EvaluateMissingCook(snapshot, rule, result);
			else if (rule.id == "cook.editor-only-leak") EvaluateEditorLeak(snapshot, rule, result);
			else if (rule.id == "cook.rule-conflict") EvaluateCookRuleConflict(snapshot, rule, result);
			else if (rule.id == "redirector.present") EvaluateRedirector(snapshot, rule, result);
			else if (rule.id == "reference.missing") EvaluateMissingReferences(snapshot, rule, result);
			else if (rule.id == "naming.ambiguous") EvaluateAmbiguousNames(snapshot, rule, result);
			else if (rule.id == "budget.directory") EvaluateDirectoryBudget(snapshot, rule, result);
			else if (rule.id == "budget.type") EvaluateTypeBudget(snapshot, rule, result);
			else if (rule.id.starts_with("budget.project")) EvaluateProjectBudget(snapshot, rule, result);
			else if (rule.id.starts_with("budget.")) EvaluateBudget(snapshot, rule, result);
			else result.diagnostics.push_back({rule.id, {}, AnalysisDiagnosticCode::UnsupportedRule, "rule is not implemented"});
		}
		std::sort(result.findings.begin(), result.findings.end(), [](const Finding& left, const Finding& right) {
			return left.ruleId < right.ruleId ||
				(left.ruleId == right.ruleId && (left.assetPath < right.assetPath ||
					(left.assetPath == right.assetPath && left.message < right.message)));
		});
		std::sort(result.diagnostics.begin(), result.diagnostics.end(), [](const AnalysisDiagnostic& left, const AnalysisDiagnostic& right) {
			return left.ruleId < right.ruleId ||
				(left.ruleId == right.ruleId && (left.assetPath < right.assetPath ||
					(left.assetPath == right.assetPath && left.message < right.message)));
		});
		return result;
	}
}
