#pragma once

#include "cookscope/graph.h"
#include "cookscope/rule_config.h"
#include "cookscope/snapshot.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cookscope
{
	enum class FindingBaselineState : std::uint8_t
	{
		NotApplicable,
		New,
		Existing,
		Worsened,
	};

	struct Finding
	{
		std::string ruleId;
		std::string assetPath;
		Severity severity = Severity::Warning;
		std::string message;
		MeasurementKind measurementKind = MeasurementKind::Unavailable;
		std::optional<std::uint64_t> observedBytes;
		std::optional<std::uint64_t> budgetBytes;
		std::string relatedAsset;
		std::optional<DependencyKind> dependencyKind;
		std::vector<DependencyStep> dependencyPath;
		std::string metric;
		std::optional<std::uint64_t> observedValue;
		std::optional<std::uint64_t> limitValue;
		std::string observedText;
		std::string expectedText;
		FindingBaselineState baselineState = FindingBaselineState::NotApplicable;
	};

	enum class AnalysisDiagnosticCode : std::uint8_t
	{
		MeasurementUnavailable,
		InvalidRuleParameters,
		UnsupportedRule,
	};

	struct AnalysisDiagnostic
	{
		std::string ruleId;
		std::string assetPath;
		AnalysisDiagnosticCode code = AnalysisDiagnosticCode::InvalidRuleParameters;
		std::string message;
	};

	struct AnalysisResult
	{
		std::vector<Finding> findings;
		std::vector<AnalysisDiagnostic> diagnostics;
	};

	[[nodiscard]] COOKSCOPECORE_API bool GlobMatches(std::string_view pattern, std::string_view value);
	[[nodiscard]] COOKSCOPECORE_API AnalysisResult Evaluate(const Snapshot& snapshot, const RuleConfig& config);
	[[nodiscard]] COOKSCOPECORE_API AnalysisResult Evaluate(
		const Snapshot& candidate,
		const RuleConfig& config,
		const Snapshot* baseline);
}
