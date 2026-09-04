#include "cookscope/rule_config.h"

#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
	int Fail(std::string_view message)
	{
		std::cerr << "FAIL: " << message << '\n';
		return 1;
	}

	std::string Rule(std::string_view id, std::string_view severity, std::string_view failThreshold, std::string_view parameters)
	{
		return "{\"id\":\"" + std::string(id) +
			"\",\"name\":\"Rule name\",\"description\":\"Rule description\","
			"\"severity\":\"" + std::string(severity) +
			"\",\"scope\":{\"include\":[\"/Game/**\"],\"exclude\":[\"/Game/Developers/**\"]},"
			"\"parameters\":" + std::string(parameters) +
			",\"exceptions\":[{\"selector\":\"/Game/Allowed/**\",\"reason\":\"Reviewed fixture\"}],"
			"\"baseline\":\"report-new-or-worsened\",\"failThreshold\":\"" + std::string(failThreshold) +
			"\",\"helpUri\":\"docs/rules/example.md\"}";
	}

	std::string Config(std::string rules, std::string_view extra = {})
	{
		return "{\"schema\":\"cookscope.rules/1\",\"rules\":[" + std::move(rules) + "]" + std::string(extra) + "}";
	}
}

int main()
{
	using cookscope::ConfigErrorCode;

	const std::string zRule = Rule("z.rule", "warning", "error", "{\"budgetBytes\":2048}");
	const std::string aRule = Rule("a.rule", "error", "warning", "{\"classPrefixes\":{\"Texture2D\":\"T_\"}}");
	const auto first = cookscope::ParseRuleConfig(Config(zRule + "," + aRule));
	const auto second = cookscope::ParseRuleConfig(Config(aRule + "," + zRule));
	if (!first.ok || !second.ok)
	{
		return Fail("valid strict rule configurations must parse");
	}
	const std::string firstCanonical = cookscope::WriteCanonicalRuleConfig(first.value);
	const std::string secondCanonical = cookscope::WriteCanonicalRuleConfig(second.value);
	if (firstCanonical != secondCanonical)
	{
		return Fail("rule input order must not change canonical bytes");
	}
	if (firstCanonical.find("\"id\":\"a.rule\"") > firstCanonical.find("\"id\":\"z.rule\"") || firstCanonical.back() != '\n')
	{
		return Fail("canonical rules must sort by ID and end with LF");
	}

	const auto duplicate = cookscope::ParseRuleConfig(Config(aRule + "," + aRule));
	if (duplicate.ok || duplicate.error.code != ConfigErrorCode::DuplicateRuleId || duplicate.error.path != "$.rules[1].id")
	{
		return Fail("duplicate Rule IDs must fail closed at a stable path");
	}

	const auto unknownRoot = cookscope::ParseRuleConfig(Config(aRule, ",\"unexpected\":true"));
	if (unknownRoot.ok || unknownRoot.error.code != ConfigErrorCode::UnknownField || unknownRoot.error.path != "$.unexpected")
	{
		return Fail("unknown root fields must fail closed");
	}

	std::string unknownRule = aRule;
	unknownRule.insert(unknownRule.size() - 1, ",\"surprise\":true");
	const auto unknownRuleResult = cookscope::ParseRuleConfig(Config(unknownRule));
	if (unknownRuleResult.ok || unknownRuleResult.error.code != ConfigErrorCode::UnknownField ||
		unknownRuleResult.error.path != "$.rules[0].surprise")
	{
		return Fail("unknown rule fields must fail closed");
	}

	const auto invalidSeverity = cookscope::ParseRuleConfig(Config(Rule("bad.severity", "critical", "error", "{}")));
	if (invalidSeverity.ok || invalidSeverity.error.code != ConfigErrorCode::InvalidValue)
	{
		return Fail("unknown severity must fail closed");
	}

	const auto invalidThreshold = cookscope::ParseRuleConfig(Config(Rule("bad.threshold", "error", "critical", "{}")));
	if (invalidThreshold.ok || invalidThreshold.error.code != ConfigErrorCode::InvalidValue)
	{
		return Fail("unknown fail threshold must fail closed");
	}

	const auto negativeBudget = cookscope::ParseRuleConfig(Config(Rule("bad.budget", "error", "error", "{\"budgetBytes\":-1}")));
	if (negativeBudget.ok || negativeBudget.error.code != ConfigErrorCode::InvalidValue ||
		negativeBudget.error.path != "$.rules[0].parameters.budgetBytes")
	{
		return Fail("negative byte budget must fail closed at a stable path");
	}

	std::string duplicateExceptionRule = aRule;
	const std::string oneException = "[{\"selector\":\"/Game/Allowed/**\",\"reason\":\"Reviewed fixture\"}]";
	const std::string twoExceptions = "[{\"selector\":\"/Game/Allowed/**\",\"reason\":\"Reviewed fixture\"},{\"selector\":\"/Game/Allowed/**\",\"reason\":\"Duplicate\"}]";
	duplicateExceptionRule.replace(duplicateExceptionRule.find(oneException), oneException.size(), twoExceptions);
	const auto duplicateException = cookscope::ParseRuleConfig(Config(duplicateExceptionRule));
	if (duplicateException.ok || duplicateException.error.code != ConfigErrorCode::InvalidValue ||
		duplicateException.error.path != "$.rules[0].exceptions[1].selector")
	{
		return Fail("duplicate exception selectors must fail closed at a stable path");
	}

	std::cout << "PASS: strict versioned Rule Config contract\n";
	return 0;
}
