#include "cookscope/bootstrap.h"

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
	using cookscope::AuditStatus;

	if (cookscope::CommandletExitCode(AuditStatus::Clean) != 0)
	{
		return Fail("clean status must map to exit code 0");
	}
	if (cookscope::CommandletExitCode(AuditStatus::Violation) != 2)
	{
		return Fail("violation status must map to exit code 2");
	}
	if (cookscope::CommandletExitCode(AuditStatus::InvalidInvocation) != 3)
	{
		return Fail("invalid invocation must map to exit code 3");
	}
	if (cookscope::CommandletExitCode(AuditStatus::InternalError) != 4)
	{
		return Fail("internal error must map to exit code 4");
	}
	if (cookscope::CommandletExitCode(AuditStatus::Cancelled) != 5)
	{
		return Fail("cancelled status must map to exit code 5");
	}

	const cookscope::BootstrapReport report{AuditStatus::Violation, 2, 1};
	const std::string expected =
		"{\"schema\":\"cookscope.result/1\",\"status\":\"violation\","
		"\"summary\":{\"errors\":2,\"warnings\":1}}\n";
	if (cookscope::WriteBootstrapJson(report) != expected)
	{
		return Fail("bootstrap JSON must be canonical and byte-stable");
	}

	std::cout << "PASS: bootstrap status, exit code, and canonical JSON contract\n";
	return 0;
}
