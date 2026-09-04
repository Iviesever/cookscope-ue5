#include "cookscope/arguments.h"

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
}
