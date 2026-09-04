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
		MissingConfig,
		MissingOutput,
		MissingSourceSha,
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

	struct AuditArguments
	{
		std::string configPath;
		std::string outputDirectory;
		std::string sourceSha;
		std::string scope = "/Game";
		std::string baselinePath;
		std::string cookRegistryPath;
		std::string cookPlatform = "Windows";
		std::string cookConfiguration = "Development";
		bool failOnViolation = true;
		std::uint32_t timeoutSeconds = 300;
	};

	struct AuditArgumentsResult
	{
		bool ok = false;
		AuditArguments value;
		ArgumentError error = ArgumentError::None;
		std::string message;
	};

	[[nodiscard]] COOKSCOPECORE_API BootstrapArgumentsResult ParseBootstrapArguments(
		std::span<const std::string_view> arguments);
	[[nodiscard]] COOKSCOPECORE_API AuditArgumentsResult ParseAuditArguments(
		std::span<const std::string_view> arguments);
}
