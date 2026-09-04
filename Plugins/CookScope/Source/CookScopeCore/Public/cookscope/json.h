#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#ifndef COOKSCOPECORE_API
#define COOKSCOPECORE_API
#endif

namespace cookscope
{
	enum class JsonType : std::uint8_t
	{
		Null,
		Boolean,
		Number,
		String,
		Array,
		Object,
	};

	struct JsonValue
	{
		JsonType type = JsonType::Null;
		bool boolean = false;
		std::string scalar;
		std::vector<JsonValue> array;
		std::map<std::string, JsonValue, std::less<>> object;
	};

	enum class JsonErrorCode : std::uint8_t
	{
		None,
		UnexpectedEnd,
		UnexpectedToken,
		InvalidString,
		InvalidEscape,
		InvalidUnicode,
		InvalidNumber,
		DuplicateKey,
		TrailingContent,
		DepthExceeded,
	};

	struct JsonError
	{
		JsonErrorCode code = JsonErrorCode::None;
		std::size_t offset = 0;
		std::string path = "$";
		std::string message;
	};

	struct JsonParseResult
	{
		bool ok = false;
		JsonValue value;
		JsonError error;
	};

	[[nodiscard]] COOKSCOPECORE_API JsonParseResult ParseJson(
		std::string_view utf8Json,
		std::size_t maximumDepth = 128);
	[[nodiscard]] COOKSCOPECORE_API std::string WriteCanonicalJson(const JsonValue& value);
}

