#pragma once

#include <cstdint>
#include <string>

#ifndef COOKSCOPECORE_API
#define COOKSCOPECORE_API
#endif

namespace cookscope
{
	enum class AuditStatus : std::uint8_t
	{
		Clean,
		Violation,
		InvalidInvocation,
		InternalError,
		Cancelled,
	};

	struct BootstrapReport
	{
		AuditStatus status = AuditStatus::Clean;
		std::uint64_t errors = 0;
		std::uint64_t warnings = 0;
	};

	[[nodiscard]] COOKSCOPECORE_API int CommandletExitCode(AuditStatus status) noexcept;
	[[nodiscard]] COOKSCOPECORE_API std::string WriteBootstrapJson(const BootstrapReport& report);
}
