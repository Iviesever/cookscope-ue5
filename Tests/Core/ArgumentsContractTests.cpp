#include "cookscope/arguments.h"

#include <array>
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
	using namespace std::string_view_literals;
	using cookscope::ArgumentError;
	using cookscope::AuditStatus;

	const std::array<std::string_view, 0> none{};
	const auto missingOutput = cookscope::ParseBootstrapArguments(none);
	if (missingOutput.ok || missingOutput.error != ArgumentError::MissingOutput)
	{
		return Fail("output must be required");
	}

	const std::array unknown{"-output=Artifacts/report.json"sv, "-surprise=true"sv};
	const auto unknownResult = cookscope::ParseBootstrapArguments(unknown);
	if (unknownResult.ok || unknownResult.error != ArgumentError::UnknownArgument ||
		unknownResult.message != "unknown argument: -surprise=true")
	{
		return Fail("unknown arguments must fail closed with a stable message");
	}

	const std::array duplicate{"-output=one.json"sv, "-output=two.json"sv};
	const auto duplicateResult = cookscope::ParseBootstrapArguments(duplicate);
	if (duplicateResult.ok || duplicateResult.error != ArgumentError::DuplicateArgument)
	{
		return Fail("duplicate arguments must fail closed");
	}

	const std::array valid{"-output=Artifacts/report.json"sv, "-fixture-status=violation"sv};
	const auto validResult = cookscope::ParseBootstrapArguments(valid);
	if (!validResult.ok || validResult.value.outputPath != "Artifacts/report.json" ||
		validResult.value.status != AuditStatus::Violation)
	{
		return Fail("valid arguments must preserve output and status");
	}

	std::cout << "PASS: strict bootstrap argument contract\n";
	return 0;
}
