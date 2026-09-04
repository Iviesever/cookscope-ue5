#include "cookscope/arguments.h"
#include "cookscope/bootstrap.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace
{
	int WriteReport(const cookscope::BootstrapArguments& arguments)
	{
		const std::u8string outputUtf8(arguments.outputPath.begin(), arguments.outputPath.end());
		const std::filesystem::path outputPath(outputUtf8);
		std::error_code error;
		if (outputPath.has_parent_path())
		{
			std::filesystem::create_directories(outputPath.parent_path(), error);
			if (error)
			{
				std::cerr << "failed to create output directory: " << error.message() << '\n';
				return cookscope::CommandletExitCode(cookscope::AuditStatus::InternalError);
			}
		}

		const std::uint64_t errorCount = arguments.status == cookscope::AuditStatus::Violation ? 1 : 0;
		const cookscope::BootstrapReport report{arguments.status, errorCount, 0};
		const std::string json = cookscope::WriteBootstrapJson(report);
		std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
		output.write(json.data(), static_cast<std::streamsize>(json.size()));
		output.close();
		if (!output)
		{
			std::cerr << "failed to write output report\n";
			return cookscope::CommandletExitCode(cookscope::AuditStatus::InternalError);
		}

		return cookscope::CommandletExitCode(arguments.status);
	}
}

int main(int argc, char** argv)
{
	std::vector<std::string_view> argumentViews;
	argumentViews.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0);
	for (int index = 1; index < argc; ++index)
	{
		argumentViews.emplace_back(argv[index]);
	}

	const auto parsed = cookscope::ParseBootstrapArguments(std::span<const std::string_view>(argumentViews));
	if (!parsed.ok)
	{
		std::cerr << parsed.message << '\n';
		return cookscope::CommandletExitCode(cookscope::AuditStatus::InvalidInvocation);
	}

	return WriteReport(parsed.value);
}
