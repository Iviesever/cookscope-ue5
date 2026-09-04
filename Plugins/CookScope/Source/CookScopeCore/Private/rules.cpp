#include "cookscope/rules.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
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
