#pragma once

#include "cookscope/json.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace cookscope
{
	enum class Severity : std::uint8_t
	{
		Note,
		Warning,
		Error,
	};

	enum class BaselineBehavior : std::uint8_t
	{
		ReportAll,
		ReportNewOrWorsened,
		SuppressExisting,
	};

	struct RuleScope
	{
		std::vector<std::string> include;
		std::vector<std::string> exclude;
	};

	struct RuleException
	{
		std::string selector;
		std::string reason;
	};

	struct RuleDefinition
	{
		std::string id;
		std::string name;
		std::string description;
		Severity severity = Severity::Warning;
		RuleScope scope;
		JsonValue parameters;
		std::vector<RuleException> exceptions;
		BaselineBehavior baseline = BaselineBehavior::ReportNewOrWorsened;
		Severity failThreshold = Severity::Error;
		std::string helpUri;
	};

	struct RuleConfig
	{
		std::string schema = "cookscope.rules/1";
		std::vector<RuleDefinition> rules;
	};

	enum class ConfigErrorCode : std::uint8_t
	{
		None,
		JsonSyntax,
		ExpectedObject,
		MissingField,
		UnknownField,
		InvalidType,
		InvalidValue,
		DuplicateRuleId,
	};

	struct ConfigError
	{
		ConfigErrorCode code = ConfigErrorCode::None;
		std::string path = "$";
		std::string message;
	};

	struct RuleConfigParseResult
	{
		bool ok = false;
		RuleConfig value;
		ConfigError error;
	};

	[[nodiscard]] COOKSCOPECORE_API RuleConfigParseResult ParseRuleConfig(std::string_view utf8Json);
	[[nodiscard]] COOKSCOPECORE_API std::string WriteCanonicalRuleConfig(const RuleConfig& config);
}

