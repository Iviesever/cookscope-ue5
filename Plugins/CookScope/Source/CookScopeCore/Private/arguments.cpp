#include "cookscope/arguments.h"

#include <algorithm>
#include <charconv>

namespace cookscope
{
	namespace
	{
		BootstrapArgumentsResult Failure(ArgumentError error, std::string message)
		{
			BootstrapArgumentsResult result;
			result.error = error;
			result.message = std::move(message);
			return result;
		}

		bool ParseStatus(std::string_view value, AuditStatus& status) noexcept
		{
			if (value == "clean")
			{
				status = AuditStatus::Clean;
				return true;
			}
			if (value == "violation")
			{
				status = AuditStatus::Violation;
				return true;
			}
			if (value == "internal-error")
			{
				status = AuditStatus::InternalError;
				return true;
			}
			if (value == "cancelled")
			{
				status = AuditStatus::Cancelled;
				return true;
			}
			return false;
		}

		AuditArgumentsResult AuditFailure(ArgumentError error, std::string message)
		{
			AuditArgumentsResult result;
			result.error = error;
			result.message = std::move(message);
			return result;
		}

		bool IsSha(std::string_view value)
		{
			return value.size() == 40 && std::all_of(value.begin(), value.end(), [](unsigned char character) {
				return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f') ||
					(character >= 'A' && character <= 'F');
			});
		}

		enum class AuditStringStatus : std::uint8_t
		{
			NoMatch,
			Success,
			Failure,
		};

		AuditStringStatus ReadAuditString(
			std::string_view argument,
			std::string_view prefix,
			bool& seen,
			std::string& output,
			std::string_view name,
			AuditArgumentsResult& failure)
		{
			if (!argument.starts_with(prefix)) return AuditStringStatus::NoMatch;
			if (seen)
			{
				failure = AuditFailure(ArgumentError::DuplicateArgument, "duplicate argument: " + std::string(name));
				return AuditStringStatus::Failure;
			}
			const std::string_view value = argument.substr(prefix.size());
			if (value.empty())
			{
				failure = AuditFailure(ArgumentError::InvalidValue, "empty value: " + std::string(name));
				return AuditStringStatus::Failure;
			}
			seen = true;
			output.assign(value);
			return AuditStringStatus::Success;
		}
	}

	BootstrapArgumentsResult ParseBootstrapArguments(std::span<const std::string_view> arguments)
	{
		constexpr std::string_view OutputPrefix = "-output=";
		constexpr std::string_view StatusPrefix = "-fixture-status=";
		BootstrapArguments parsed;
		bool hasOutput = false;
		bool hasStatus = false;

		for (const std::string_view argument : arguments)
		{
			if (argument.starts_with(OutputPrefix))
			{
				if (hasOutput)
				{
					return Failure(ArgumentError::DuplicateArgument, "duplicate argument: -output");
				}
				const std::string_view value = argument.substr(OutputPrefix.size());
				if (value.empty())
				{
					return Failure(ArgumentError::InvalidValue, "invalid value: -output must not be empty");
				}
				parsed.outputPath.assign(value);
				hasOutput = true;
				continue;
			}

			if (argument.starts_with(StatusPrefix))
			{
				if (hasStatus)
				{
					return Failure(ArgumentError::DuplicateArgument, "duplicate argument: -fixture-status");
				}
				const std::string_view value = argument.substr(StatusPrefix.size());
				if (!ParseStatus(value, parsed.status))
				{
					return Failure(ArgumentError::InvalidValue, "invalid value for -fixture-status: " + std::string(value));
				}
				hasStatus = true;
				continue;
			}

			return Failure(ArgumentError::UnknownArgument, "unknown argument: " + std::string(argument));
		}

		if (!hasOutput)
		{
			return Failure(ArgumentError::MissingOutput, "missing required argument: -output=<path>");
		}

		BootstrapArgumentsResult result;
		result.ok = true;
		result.value = std::move(parsed);
		return result;
	}

	AuditArgumentsResult ParseAuditArguments(std::span<const std::string_view> arguments)
	{
		constexpr std::string_view ConfigPrefix = "-config=";
		constexpr std::string_view OutputPrefix = "-output=";
		constexpr std::string_view SourceShaPrefix = "-source-sha=";
		constexpr std::string_view ScopePrefix = "-scope=";
		constexpr std::string_view BaselinePrefix = "-baseline=";
		constexpr std::string_view FailPrefix = "-fail-on-violation=";
		constexpr std::string_view TimeoutPrefix = "-timeout-seconds=";
		AuditArguments parsed;
		bool hasConfig = false;
		bool hasOutput = false;
		bool hasSourceSha = false;
		bool hasScope = false;
		bool hasBaseline = false;
		bool hasFail = false;
		bool hasTimeout = false;

		for (const std::string_view argument : arguments)
		{
			AuditArgumentsResult failure;
			auto read = [&](std::string_view prefix, bool& seen, std::string& output, std::string_view name) {
				return ReadAuditString(argument, prefix, seen, output, name, failure);
			};
			AuditStringStatus status = read(ConfigPrefix, hasConfig, parsed.configPath, "-config");
			if (status == AuditStringStatus::Failure) return failure;
			if (status == AuditStringStatus::Success) continue;
			status = read(OutputPrefix, hasOutput, parsed.outputDirectory, "-output");
			if (status == AuditStringStatus::Failure) return failure;
			if (status == AuditStringStatus::Success) continue;
			status = read(SourceShaPrefix, hasSourceSha, parsed.sourceSha, "-source-sha");
			if (status == AuditStringStatus::Failure) return failure;
			if (status == AuditStringStatus::Success) continue;
			status = read(ScopePrefix, hasScope, parsed.scope, "-scope");
			if (status == AuditStringStatus::Failure) return failure;
			if (status == AuditStringStatus::Success) continue;
			status = read(BaselinePrefix, hasBaseline, parsed.baselinePath, "-baseline");
			if (status == AuditStringStatus::Failure) return failure;
			if (status == AuditStringStatus::Success) continue;

			if (argument.starts_with(FailPrefix))
			{
				if (hasFail) return AuditFailure(ArgumentError::DuplicateArgument, "duplicate argument: -fail-on-violation");
				const std::string_view value = argument.substr(FailPrefix.size());
				if (value == "true") parsed.failOnViolation = true;
				else if (value == "false") parsed.failOnViolation = false;
				else return AuditFailure(ArgumentError::InvalidValue, "-fail-on-violation must be true or false");
				hasFail = true;
				continue;
			}
			if (argument.starts_with(TimeoutPrefix))
			{
				if (hasTimeout) return AuditFailure(ArgumentError::DuplicateArgument, "duplicate argument: -timeout-seconds");
				const std::string_view value = argument.substr(TimeoutPrefix.size());
				std::uint32_t timeout = 0;
				const auto converted = std::from_chars(value.data(), value.data() + value.size(), timeout);
				if (converted.ec != std::errc{} || converted.ptr != value.data() + value.size() || timeout == 0 || timeout > 86400)
				{
					return AuditFailure(ArgumentError::InvalidValue, "-timeout-seconds must be an integer from 1 to 86400");
				}
				parsed.timeoutSeconds = timeout;
				hasTimeout = true;
				continue;
			}
			return AuditFailure(ArgumentError::UnknownArgument, "unknown argument: " + std::string(argument));
		}

		if (!hasConfig) return AuditFailure(ArgumentError::MissingConfig, "missing required argument: -config=<path>");
		if (!hasOutput) return AuditFailure(ArgumentError::MissingOutput, "missing required argument: -output=<directory>");
		if (!hasSourceSha) return AuditFailure(ArgumentError::MissingSourceSha, "missing required argument: -source-sha=<40-hex>");
		if (!IsSha(parsed.sourceSha)) return AuditFailure(ArgumentError::InvalidValue, "-source-sha must be 40 hexadecimal characters");
		std::transform(parsed.sourceSha.begin(), parsed.sourceSha.end(), parsed.sourceSha.begin(), [](unsigned char value) {
			return static_cast<char>(value >= 'A' && value <= 'F' ? value - 'A' + 'a' : value);
		});
		AuditArgumentsResult result;
		result.ok = true;
		result.value = std::move(parsed);
		return result;
	}
}
