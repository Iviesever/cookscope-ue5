#pragma once

#include "cookscope/diff.h"
#include "cookscope/rules.h"

#include <string>

namespace cookscope
{
	struct ReportSet
	{
		std::string json;
		std::string sarif;
		std::string junit;
		std::string html;
	};

	[[nodiscard]] COOKSCOPECORE_API ReportSet RenderReports(
		const Snapshot& snapshot,
		const RuleConfig& config,
		const AnalysisResult& analysis,
		const SnapshotDiffResult* diff = nullptr);
}

