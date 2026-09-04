#pragma once

#include "cookscope/bootstrap.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace cookscope
{
	enum class ArgumentError : std::uint8_t
	{
		None,
		MissingOutput,
		UnknownArgument,
		DuplicateArgument,
		InvalidValue,
	};

	struct BootstrapArguments
	{
		std::string outputPath;
		AuditStatus status = AuditStatus::Clean;
	};

	struct BootstrapArgumentsResult
	{
		bool ok = false;
		BootstrapArguments value;
		ArgumentError error = ArgumentError::None;
		std::string message;
	};

	[[nodiscard]] COOKSCOPECORE_API BootstrapArgumentsResult ParseBootstrapArguments(
		std::span<const std::string_view> arguments);
}
