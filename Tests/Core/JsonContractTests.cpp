#include "cookscope/json.h"

#include <iostream>
#include <string_view>

namespace
{
	int Fail(std::string_view message)
	{
		std::cerr << "FAIL: " << message << '\n';
		return 1;
	}
}

int main()
{
	using cookscope::JsonErrorCode;

	const auto parsed = cookscope::ParseJson(R"({"z":2,"message":"资产","a":[true,null,"x"]})");
	if (!parsed.ok)
	{
		return Fail("valid JSON must parse");
	}
	if (cookscope::WriteCanonicalJson(parsed.value) != R"({"a":[true,null,"x"],"message":"资产","z":2})")
	{
		return Fail("canonical JSON must sort object keys and preserve UTF-8");
	}

	const auto reordered = cookscope::ParseJson(R"({"a":[true,null,"x"],"z":2,"message":"资产"})");
	if (!reordered.ok || cookscope::WriteCanonicalJson(reordered.value) != cookscope::WriteCanonicalJson(parsed.value))
	{
		return Fail("equivalent object order must produce identical bytes");
	}

	const auto duplicate = cookscope::ParseJson(R"({"id":"first","id":"second"})");
	if (duplicate.ok || duplicate.error.code != JsonErrorCode::DuplicateKey || duplicate.error.path != "$.id")
	{
		return Fail("duplicate keys must fail closed at a stable path");
	}

	const auto trailing = cookscope::ParseJson(R"({"ok":true} false)");
	if (trailing.ok || trailing.error.code != JsonErrorCode::TrailingContent)
	{
		return Fail("trailing content must fail closed");
	}

	const auto invalidSurrogate = cookscope::ParseJson(R"({"text":"\uD800"})");
	if (invalidSurrogate.ok || invalidSurrogate.error.code != JsonErrorCode::InvalidUnicode)
	{
		return Fail("unpaired UTF-16 surrogate must fail closed");
	}

	std::cout << "PASS: strict JSON parser and canonical writer contract\n";
	return 0;
}
