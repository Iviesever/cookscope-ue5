#include "cookscope/rules.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <functional>
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
