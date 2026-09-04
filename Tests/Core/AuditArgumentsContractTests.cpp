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

	const std::array<std::string_view, 0> none{};
	const auto missing = cookscope::ParseAuditArguments(none);
	if (missing.ok || missing.error != ArgumentError::MissingConfig)
	{
		return Fail("full audit config must be required");
	}

	const std::array valid{
		"-config=Config/CookScopeRules.json"sv,
		"-output=Artifacts/Reports"sv,
		"-source-sha=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv,
		"-scope=/Game/CookScopeFixtures"sv,
		"-baseline=baseline.json"sv,
		"-cook-registry=DevelopmentAssetRegistry.bin"sv,
		"-cook-platform=Windows"sv,
		"-cook-configuration=Development"sv,
		"-fail-on-violation=false"sv,
		"-timeout-seconds=90"sv,
	};
	const auto parsed = cookscope::ParseAuditArguments(valid);
	if (!parsed.ok || parsed.value.configPath != "Config/CookScopeRules.json" ||
		parsed.value.outputDirectory != "Artifacts/Reports" || parsed.value.scope != "/Game/CookScopeFixtures" ||
		parsed.value.baselinePath != "baseline.json" || parsed.value.failOnViolation || parsed.value.timeoutSeconds != 90 ||
		parsed.value.cookRegistryPath != "DevelopmentAssetRegistry.bin" || parsed.value.cookPlatform != "Windows" ||
		parsed.value.cookConfiguration != "Development" ||
		parsed.value.sourceSha != "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
	{
		return Fail("valid full audit arguments must preserve every value");
	}

	const std::array duplicate{
		"-config=a.json"sv, "-config=b.json"sv, "-output=out"sv,
		"-source-sha=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv};
	if (cookscope::ParseAuditArguments(duplicate).error != ArgumentError::DuplicateArgument)
	{
		return Fail("duplicate full audit arguments must fail closed");
	}

	const std::array badTimeout{
		"-config=a.json"sv, "-output=out"sv, "-source-sha=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv,
		"-timeout-seconds=0"sv};
	if (cookscope::ParseAuditArguments(badTimeout).error != ArgumentError::InvalidValue)
	{
		return Fail("zero timeout must fail closed");
	}

	const std::array badSha{
		"-config=a.json"sv, "-output=out"sv, "-source-sha=not-a-sha"sv};
	if (cookscope::ParseAuditArguments(badSha).error != ArgumentError::InvalidValue)
	{
		return Fail("invalid source SHA must fail closed");
	}

	const std::array unknown{
		"-config=a.json"sv, "-output=out"sv, "-source-sha=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"sv,
		"-surprise=true"sv};
	if (cookscope::ParseAuditArguments(unknown).error != ArgumentError::UnknownArgument)
	{
		return Fail("unknown full audit arguments must fail closed");
	}

	std::cout << "PASS: strict full audit argument contract\n";
	return 0;
}
