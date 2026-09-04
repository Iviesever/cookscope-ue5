#include "cookscope/json.h"
#include "cookscope/rule_config.h"
#include "cookscope/snapshot.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace
{
	int Fail(std::string_view message)
	{
		std::cerr << "FAIL: " << message << '\n';
		return 1;
	}

	std::string Read(std::string_view path)
	{
		std::ifstream input(std::string(path), std::ios::binary);
		return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
	}

	bool HasSchemaId(const cookscope::JsonValue& root, std::string_view expected)
	{
		if (root.type != cookscope::JsonType::Object)
		{
			return false;
		}
		const auto id = root.object.find("$id");
		return id != root.object.end() && id->second.type == cookscope::JsonType::String && id->second.scalar == expected;
	}
}

int main()
{
	const std::string ruleSchemaText = Read("Schemas/cookscope-rules.schema.json");
	const std::string snapshotSchemaText = Read("Schemas/cookscope-snapshot.schema.json");
	const std::string ruleExampleText = Read("Examples/rules/cookscope-rules.json");
	const std::string snapshotExampleText = Read("Examples/snapshots/cookscope-snapshot.json");
	if (ruleSchemaText.empty() || snapshotSchemaText.empty() || ruleExampleText.empty() || snapshotExampleText.empty())
	{
		return Fail("all public Schema and example files must exist and be non-empty");
	}

	const auto ruleSchema = cookscope::ParseJson(ruleSchemaText);
	const auto snapshotSchema = cookscope::ParseJson(snapshotSchemaText);
	if (!ruleSchema.ok || !snapshotSchema.ok ||
		!HasSchemaId(ruleSchema.value, "https://github.com/Iviesever/cookscope-ue5/schemas/cookscope-rules.schema.json") ||
		!HasSchemaId(snapshotSchema.value, "https://github.com/Iviesever/cookscope-ue5/schemas/cookscope-snapshot.schema.json"))
	{
		return Fail("public Schema files must be valid JSON with stable canonical IDs");
	}

	const auto rules = cookscope::ParseRuleConfig(ruleExampleText);
	if (!rules.ok || !cookscope::ParseRuleConfig(cookscope::WriteCanonicalRuleConfig(rules.value)).ok)
	{
		return Fail("public Rule Config example must round-trip through the strict Core");
	}
	const auto snapshot = cookscope::ParseSnapshot(snapshotExampleText);
	if (!snapshot.ok || !cookscope::ParseSnapshot(cookscope::WriteCanonicalSnapshot(snapshot.value)).ok)
	{
		return Fail("public Asset Snapshot example must round-trip through the strict Core");
	}

	std::cout << "PASS: public Schema and canonical example artifacts\n";
	return 0;
}
