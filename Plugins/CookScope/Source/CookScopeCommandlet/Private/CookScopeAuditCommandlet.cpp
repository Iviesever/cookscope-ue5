#include "CookScopeAuditCommandlet.h"

#include "cookscope/arguments.h"
#include "cookscope/bootstrap.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

DEFINE_LOG_CATEGORY_STATIC(LogCookScopeCommandlet, Log, All);

namespace
{
	bool IsEngineOwnedSwitch(const FString& Switch)
	{
		return Switch.Equals(TEXT("run=CookScopeAudit"), ESearchCase::IgnoreCase) ||
			Switch.Equals(TEXT("unattended"), ESearchCase::IgnoreCase) ||
			Switch.Equals(TEXT("nop4"), ESearchCase::IgnoreCase) ||
			Switch.Equals(TEXT("nosplash"), ESearchCase::IgnoreCase) ||
			Switch.Equals(TEXT("nullrhi"), ESearchCase::IgnoreCase) ||
			Switch.Equals(TEXT("nosound"), ESearchCase::IgnoreCase) ||
			Switch.Equals(TEXT("stdout"), ESearchCase::IgnoreCase) ||
			Switch.Equals(TEXT("fullstdoutlogoutput"), ESearchCase::IgnoreCase);
	}
}

UCookScopeAuditCommandlet::UCookScopeAuditCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
	ShowErrorCount = true;
	UseCommandletResultAsExitCode = true;
}

int32 UCookScopeAuditCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);
	if (!Tokens.IsEmpty())
	{
		UE_LOG(LogCookScopeCommandlet, Error, TEXT("Unexpected positional argument: %s"), *Tokens[0]);
		return cookscope::CommandletExitCode(cookscope::AuditStatus::InvalidInvocation);
	}

	std::vector<std::string> storage;
	storage.reserve(Switches.Num());
	for (const FString& Switch : Switches)
	{
		if (IsEngineOwnedSwitch(Switch))
		{
			continue;
		}
		FTCHARToUTF8 Converted(*Switch);
		storage.emplace_back("-" + std::string(Converted.Get(), static_cast<std::size_t>(Converted.Length())));
	}

	std::vector<std::string_view> views;
	views.reserve(storage.size());
	for (const std::string& Argument : storage)
	{
		views.emplace_back(Argument);
	}

	const cookscope::BootstrapArgumentsResult Parsed = cookscope::ParseBootstrapArguments(
		std::span<const std::string_view>(views));
	if (!Parsed.ok)
	{
		UE_LOG(LogCookScopeCommandlet, Error, TEXT("%s"), UTF8_TO_TCHAR(Parsed.message.c_str()));
		return cookscope::CommandletExitCode(cookscope::AuditStatus::InvalidInvocation);
	}

	const FString OutputPath = FPaths::ConvertRelativePathToFull(UTF8_TO_TCHAR(Parsed.value.outputPath.c_str()));
	const FString OutputDirectory = FPaths::GetPath(OutputPath);
	if (!OutputDirectory.IsEmpty() && !IFileManager::Get().MakeDirectory(*OutputDirectory, true))
	{
		UE_LOG(LogCookScopeCommandlet, Error, TEXT("Unable to create output directory: %s"), *OutputDirectory);
		return cookscope::CommandletExitCode(cookscope::AuditStatus::InternalError);
	}

	const uint64 ErrorCount = Parsed.value.status == cookscope::AuditStatus::Violation ? 1 : 0;
	const cookscope::BootstrapReport Report{Parsed.value.status, ErrorCount, 0};
	const std::string Json = cookscope::WriteBootstrapJson(Report);
	const FString JsonText(UTF8_TO_TCHAR(Json.c_str()));
	if (!FFileHelper::SaveStringToFile(
		JsonText,
		*OutputPath,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogCookScopeCommandlet, Error, TEXT("Unable to write output report: %s"), *OutputPath);
		return cookscope::CommandletExitCode(cookscope::AuditStatus::InternalError);
	}

	UE_LOG(LogCookScopeCommandlet, Display, TEXT("CookScope bootstrap report: %s"), *OutputPath);
	return cookscope::CommandletExitCode(Parsed.value.status);
}
