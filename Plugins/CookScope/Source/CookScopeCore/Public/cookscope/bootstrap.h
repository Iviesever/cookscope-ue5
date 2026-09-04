#pragma once

#include <cstdint>
#include <string>

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

	[[nodiscard]] int CommandletExitCode(AuditStatus status) noexcept;
	[[nodiscard]] std::string WriteBootstrapJson(const BootstrapReport& report);
}

