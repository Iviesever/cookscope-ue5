#include "cookscope/rule_config.h"

#include <algorithm>
#include <charconv>
#include <set>
#include <utility>

namespace cookscope
{
	namespace
	{
		class RuleConfigReader
		{
		public:
			RuleConfigParseResult Read(std::string_view json)
			{
				RuleConfigParseResult result;
				JsonParseResult syntax = ParseJson(json);
				if (!syntax.ok)
				{
					result.error = {ConfigErrorCode::JsonSyntax, syntax.error.path, syntax.error.message};
					return result;
				}
				if (syntax.value.type != JsonType::Object)
				{
					result.error = {ConfigErrorCode::ExpectedObject, "$", "rule configuration root must be an object"};
					return result;
				}

				RuleConfig config;
				if (!CheckFields(syntax.value, {"schema", "rules"}, "$"))
				{
					result.error = std::move(Error);
					return result;
				}
				const JsonValue* schema = Require(syntax.value, "schema", "$", JsonType::String);
				const JsonValue* rules = Require(syntax.value, "rules", "$", JsonType::Array);
				if (!schema || !rules)
				{
					result.error = std::move(Error);
					return result;
				}
				if (schema->scalar != "cookscope.rules/1")
				{
					Fail(ConfigErrorCode::InvalidValue, "$.schema", "unsupported rule configuration schema");
					result.error = std::move(Error);
					return result;
				}

				std::set<std::string, std::less<>> identifiers;
				for (std::size_t index = 0; index < rules->array.size(); ++index)
				{
					RuleDefinition rule;
					const std::string path = "$.rules[" + std::to_string(index) + "]";
					if (!ReadRule(rules->array[index], path, rule))
					{
						result.error = std::move(Error);
						return result;
					}
					if (!identifiers.insert(rule.id).second)
					{
						Fail(ConfigErrorCode::DuplicateRuleId, path + ".id", "duplicate Rule ID");
						result.error = std::move(Error);
						return result;
					}
					config.rules.push_back(std::move(rule));
				}

				result.ok = true;
				result.value = std::move(config);
				return result;
			}

		private:
			bool ReadRule(const JsonValue& value, const std::string& path, RuleDefinition& output)
			{
				if (value.type != JsonType::Object)
				{
					return Fail(ConfigErrorCode::ExpectedObject, path, "rule must be an object");
				}
				if (!CheckFields(value, {
					"baseline", "description", "exceptions", "failThreshold", "helpUri",
					"id", "name", "parameters", "scope", "severity"
				}, path))
				{
					return false;
				}

				const JsonValue* id = Require(value, "id", path, JsonType::String);
				const JsonValue* name = Require(value, "name", path, JsonType::String);
				const JsonValue* description = Require(value, "description", path, JsonType::String);
				const JsonValue* severity = Require(value, "severity", path, JsonType::String);
				const JsonValue* scope = Require(value, "scope", path, JsonType::Object);
				const JsonValue* parameters = Require(value, "parameters", path, JsonType::Object);
				const JsonValue* exceptions = Require(value, "exceptions", path, JsonType::Array);
				const JsonValue* baseline = Require(value, "baseline", path, JsonType::String);
				const JsonValue* failThreshold = Require(value, "failThreshold", path, JsonType::String);
				const JsonValue* helpUri = Require(value, "helpUri", path, JsonType::String);
				if (!id || !name || !description || !severity || !scope || !parameters || !exceptions || !baseline || !failThreshold || !helpUri)
				{
					return false;
				}

				if (!IsRuleId(id->scalar))
				{
					return Fail(ConfigErrorCode::InvalidValue, path + ".id", "Rule ID must be a lowercase dotted identifier");
				}
				if (name->scalar.empty() || description->scalar.empty() || helpUri->scalar.empty())
				{
					return Fail(ConfigErrorCode::InvalidValue, path, "name, description, and helpUri must not be empty");
				}

				output.id = id->scalar;
				output.name = name->scalar;
				output.description = description->scalar;
				output.helpUri = helpUri->scalar;
				if (!ReadSeverity(severity->scalar, path + ".severity", output.severity) ||
					!ReadSeverity(failThreshold->scalar, path + ".failThreshold", output.failThreshold) ||
					!ReadBaseline(baseline->scalar, path + ".baseline", output.baseline) ||
					!ReadScope(*scope, path + ".scope", output.scope) ||
					!ReadExceptions(*exceptions, path + ".exceptions", output.exceptions) ||
					!ValidateParameters(*parameters, path + ".parameters"))
				{
					return false;
				}
				output.parameters = *parameters;
				return true;
			}

			bool ReadScope(const JsonValue& value, const std::string& path, RuleScope& output)
			{
				if (!CheckFields(value, {"exclude", "include"}, path))
				{
					return false;
				}
				const JsonValue* include = Require(value, "include", path, JsonType::Array);
				const JsonValue* exclude = Require(value, "exclude", path, JsonType::Array);
				if (!include || !exclude || !ReadStringArray(*include, path + ".include", output.include) ||
					!ReadStringArray(*exclude, path + ".exclude", output.exclude))
				{
					return false;
				}
				if (output.include.empty())
				{
					return Fail(ConfigErrorCode::InvalidValue, path + ".include", "include must contain at least one selector");
				}
				return true;
			}

			bool ReadExceptions(const JsonValue& value, const std::string& path, std::vector<RuleException>& output)
			{
				for (std::size_t index = 0; index < value.array.size(); ++index)
				{
					const JsonValue& item = value.array[index];
					const std::string itemPath = path + "[" + std::to_string(index) + "]";
					if (item.type != JsonType::Object)
					{
						return Fail(ConfigErrorCode::ExpectedObject, itemPath, "exception must be an object");
					}
					if (!CheckFields(item, {"reason", "selector"}, itemPath))
					{
						return false;
					}
					const JsonValue* selector = Require(item, "selector", itemPath, JsonType::String);
					const JsonValue* reason = Require(item, "reason", itemPath, JsonType::String);
					if (!selector || !reason)
					{
						return false;
					}
					if (selector->scalar.empty() || reason->scalar.empty())
					{
						return Fail(ConfigErrorCode::InvalidValue, itemPath, "exception selector and reason must not be empty");
					}
					output.push_back({selector->scalar, reason->scalar});
				}
				return true;
			}

			bool ValidateParameters(const JsonValue& parameters, const std::string& path)
			{
				const auto budget = parameters.object.find("budgetBytes");
				if (budget == parameters.object.end())
				{
					return true;
				}
				if (budget->second.type != JsonType::Number)
				{
					return Fail(ConfigErrorCode::InvalidType, path + ".budgetBytes", "budgetBytes must be an unsigned integer");
				}
				std::uint64_t value = 0;
				const std::string& number = budget->second.scalar;
				const auto parsed = std::from_chars(number.data(), number.data() + number.size(), value);
				if (parsed.ec != std::errc{} || parsed.ptr != number.data() + number.size())
				{
					return Fail(ConfigErrorCode::InvalidValue, path + ".budgetBytes", "budgetBytes must be an unsigned 64-bit integer");
				}
				return true;
			}

			bool ReadStringArray(const JsonValue& value, const std::string& path, std::vector<std::string>& output)
			{
				for (std::size_t index = 0; index < value.array.size(); ++index)
				{
					if (value.array[index].type != JsonType::String || value.array[index].scalar.empty())
					{
						return Fail(ConfigErrorCode::InvalidType, path + "[" + std::to_string(index) + "]", "selector must be a non-empty string");
					}
					output.push_back(value.array[index].scalar);
				}
				return true;
			}

			const JsonValue* Require(const JsonValue& object, std::string_view name, const std::string& path, JsonType type)
			{
				const auto found = object.object.find(name);
				if (found == object.object.end())
				{
					Fail(ConfigErrorCode::MissingField, path + "." + std::string(name), "missing required field");
					return nullptr;
				}
				if (found->second.type != type)
				{
					Fail(ConfigErrorCode::InvalidType, path + "." + std::string(name), "field has the wrong JSON type");
					return nullptr;
				}
				return &found->second;
			}

			bool CheckFields(const JsonValue& object, std::initializer_list<std::string_view> allowed, const std::string& path)
			{
				for (const auto& [name, ignored] : object.object)
				{
					(void)ignored;
					if (std::find(allowed.begin(), allowed.end(), name) == allowed.end())
					{
						return Fail(ConfigErrorCode::UnknownField, path + "." + name, "unknown field");
					}
				}
				return true;
			}

			bool ReadSeverity(std::string_view value, const std::string& path, Severity& output)
			{
				if (value == "note") output = Severity::Note;
				else if (value == "warning") output = Severity::Warning;
				else if (value == "error") output = Severity::Error;
				else return Fail(ConfigErrorCode::InvalidValue, path, "severity must be note, warning, or error");
				return true;
			}

			bool ReadBaseline(std::string_view value, const std::string& path, BaselineBehavior& output)
			{
				if (value == "report-all") output = BaselineBehavior::ReportAll;
				else if (value == "report-new-or-worsened") output = BaselineBehavior::ReportNewOrWorsened;
				else if (value == "suppress-existing") output = BaselineBehavior::SuppressExisting;
				else return Fail(ConfigErrorCode::InvalidValue, path, "invalid baseline behavior");
				return true;
			}

			bool Fail(ConfigErrorCode code, std::string path, std::string message)
			{
				if (Error.code == ConfigErrorCode::None)
				{
					Error = {code, std::move(path), std::move(message)};
				}
				return false;
			}

			static bool IsRuleId(std::string_view value)
			{
				if (value.empty() || value.front() == '.' || value.back() == '.' || value.find('.') == std::string_view::npos)
				{
					return false;
				}
				bool previousDot = false;
				for (const char character : value)
				{
					const bool valid = (character >= 'a' && character <= 'z') ||
						(character >= '0' && character <= '9') || character == '-' || character == '.';
					if (!valid || (character == '.' && previousDot))
					{
						return false;
					}
					previousDot = character == '.';
				}
				return true;
			}

			ConfigError Error;
		};

		JsonValue RuleJsonString(std::string value)
		{
			JsonValue result;
			result.type = JsonType::String;
			result.scalar = std::move(value);
			return result;
		}

		JsonValue RuleJsonStrings(const std::vector<std::string>& values)
		{
			JsonValue result;
			result.type = JsonType::Array;
			for (const std::string& value : values) result.array.push_back(RuleJsonString(value));
			return result;
		}

		std::string_view RuleSeverityName(Severity severity)
		{
			switch (severity)
			{
			case Severity::Note: return "note";
			case Severity::Warning: return "warning";
			case Severity::Error: return "error";
			}
			return "error";
		}

		std::string_view RuleBaselineName(BaselineBehavior baseline)
		{
			switch (baseline)
			{
			case BaselineBehavior::ReportAll: return "report-all";
			case BaselineBehavior::ReportNewOrWorsened: return "report-new-or-worsened";
			case BaselineBehavior::SuppressExisting: return "suppress-existing";
			}
			return "report-new-or-worsened";
		}
	}

	RuleConfigParseResult ParseRuleConfig(std::string_view utf8Json)
	{
		return RuleConfigReader().Read(utf8Json);
	}

	std::string WriteCanonicalRuleConfig(const RuleConfig& config)
	{
		std::vector<RuleDefinition> rules = config.rules;
		std::sort(rules.begin(), rules.end(), [](const RuleDefinition& left, const RuleDefinition& right) {
			return left.id < right.id;
		});

		JsonValue root;
		root.type = JsonType::Object;
		root.object.emplace("schema", RuleJsonString(config.schema));
		JsonValue ruleArray;
		ruleArray.type = JsonType::Array;
		for (RuleDefinition& rule : rules)
		{
			JsonValue value;
			value.type = JsonType::Object;
			value.object.emplace("id", RuleJsonString(rule.id));
			value.object.emplace("name", RuleJsonString(rule.name));
			value.object.emplace("description", RuleJsonString(rule.description));
			value.object.emplace("severity", RuleJsonString(std::string(RuleSeverityName(rule.severity))));

			JsonValue scope;
			scope.type = JsonType::Object;
			scope.object.emplace("include", RuleJsonStrings(rule.scope.include));
			scope.object.emplace("exclude", RuleJsonStrings(rule.scope.exclude));
			value.object.emplace("scope", std::move(scope));
			value.object.emplace("parameters", rule.parameters);

			std::sort(rule.exceptions.begin(), rule.exceptions.end(), [](const RuleException& left, const RuleException& right) {
				return left.selector < right.selector || (left.selector == right.selector && left.reason < right.reason);
			});
			JsonValue exceptions;
			exceptions.type = JsonType::Array;
			for (const RuleException& exception : rule.exceptions)
			{
				JsonValue item;
				item.type = JsonType::Object;
				item.object.emplace("selector", RuleJsonString(exception.selector));
				item.object.emplace("reason", RuleJsonString(exception.reason));
				exceptions.array.push_back(std::move(item));
			}
			value.object.emplace("exceptions", std::move(exceptions));
			value.object.emplace("baseline", RuleJsonString(std::string(RuleBaselineName(rule.baseline))));
			value.object.emplace("failThreshold", RuleJsonString(std::string(RuleSeverityName(rule.failThreshold))));
			value.object.emplace("helpUri", RuleJsonString(rule.helpUri));
			ruleArray.array.push_back(std::move(value));
		}
		root.object.emplace("rules", std::move(ruleArray));
		return WriteCanonicalJson(root) + "\n";
	}
}
