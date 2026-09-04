#include "cookscope/bootstrap.h"

#include <string_view>

namespace cookscope
{
	namespace
	{
		std::string_view StatusName(AuditStatus status) noexcept
		{
			switch (status)
			{
			case AuditStatus::Clean:
				return "clean";
			case AuditStatus::Violation:
				return "violation";
			case AuditStatus::InvalidInvocation:
				return "invalid-invocation";
			case AuditStatus::InternalError:
				return "internal-error";
			case AuditStatus::Cancelled:
				return "cancelled";
			}
			return "internal-error";
		}
	}

	int CommandletExitCode(AuditStatus status) noexcept
	{
		switch (status)
		{
		case AuditStatus::Clean:
			return 0;
		case AuditStatus::Violation:
			return 2;
		case AuditStatus::InvalidInvocation:
			return 3;
		case AuditStatus::InternalError:
			return 4;
		case AuditStatus::Cancelled:
			return 5;
		}
		return 4;
	}

	std::string WriteBootstrapJson(const BootstrapReport& report)
	{
		std::string json;
		json.reserve(128);
		json += "{\"schema\":\"cookscope.result/1\",\"status\":\"";
		json += StatusName(report.status);
		json += "\",\"summary\":{\"errors\":";
		json += std::to_string(report.errors);
		json += ",\"warnings\":";
		json += std::to_string(report.warnings);
		json += "}}\n";
		return json;
	}
}

