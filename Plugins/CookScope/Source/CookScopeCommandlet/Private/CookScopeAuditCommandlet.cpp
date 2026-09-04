#include "CookScopeAuditCommandlet.h"

#include "CookScopeAssetScanner.h"
#include "CookScopeCookSnapshotReader.h"

#include "cookscope/arguments.h"
#include "cookscope/bootstrap.h"
#include "cookscope/diff.h"
#include "cookscope/reports.h"
#include "cookscope/rule_config.h"
#include "cookscope/rules.h"
#include "cookscope/snapshot.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include <algorithm>
#include <optional>
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

	std::string CommandletToUtf8(const FString& Text)
	{
		FTCHARToUTF8 Converted(*Text);
		return std::string(Converted.Get(), static_cast<std::size_t>(Converted.Length()));
	}

	bool LoadUtf8(const FString& Path, std::string& Output)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path)) return false;
		Output = CommandletToUtf8(Text);
		return true;
	}

	bool SaveUtf8(const FString& Path, const std::string& Text)
	{
		return FFileHelper::SaveStringToFile(
			FString(UTF8_TO_TCHAR(Text.c_str())),
			*Path,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}

	bool FindingBlocks(const cookscope::Finding& Finding, const cookscope::RuleConfig& Config)
	{
		const auto Rule = std::find_if(Config.rules.begin(), Config.rules.end(), [&](const cookscope::RuleDefinition& Item) {
			return Item.id == Finding.ruleId;
		});
		return Rule != Config.rules.end() && Finding.severity >= Rule->failThreshold;
	}

	int32 RunFullAudit(std::span<const std::string_view> Arguments)
	{
		const double StartedAt = FPlatformTime::Seconds();
		const cookscope::AuditArgumentsResult Parsed = cookscope::ParseAuditArguments(Arguments);
		if (!Parsed.ok)
		{
			UE_LOG(LogCookScopeCommandlet, Error, TEXT("%s"), UTF8_TO_TCHAR(Parsed.message.c_str()));
			return cookscope::CommandletExitCode(cookscope::AuditStatus::InvalidInvocation);
		}

		const FString ConfigPath = FPaths::ConvertRelativePathToFull(UTF8_TO_TCHAR(Parsed.value.configPath.c_str()));
		std::string ConfigText;
		if (!LoadUtf8(ConfigPath, ConfigText))
		{
			UE_LOG(LogCookScopeCommandlet, Error, TEXT("Unable to read config: %s"), *ConfigPath);
			return cookscope::CommandletExitCode(cookscope::AuditStatus::InvalidInvocation);
		}
		const cookscope::RuleConfigParseResult Config = cookscope::ParseRuleConfig(ConfigText);
		if (!Config.ok)
		{
			UE_LOG(LogCookScopeCommandlet, Error, TEXT("Invalid config at %s: %s"),
				UTF8_TO_TCHAR(Config.error.path.c_str()), UTF8_TO_TCHAR(Config.error.message.c_str()));
			return cookscope::CommandletExitCode(cookscope::AuditStatus::InvalidInvocation);
		}

		const FCookScopeScanResult Scan = FCookScopeAssetScanner::ScanPath(
			UTF8_TO_TCHAR(Parsed.value.scope.c_str()),
			UTF8_TO_TCHAR(Parsed.value.sourceSha.c_str()));
		if (!Scan.bSuccess)
		{
			UE_LOG(LogCookScopeCommandlet, Error, TEXT("Asset scan failed: %s"), *Scan.Error);
			return cookscope::CommandletExitCode(cookscope::AuditStatus::InternalError);
		}
		cookscope::Snapshot Candidate = Scan.Snapshot;
		if (!Parsed.value.cookRegistryPath.empty())
		{
			const FString CookRegistryPath = FPaths::ConvertRelativePathToFull(UTF8_TO_TCHAR(Parsed.value.cookRegistryPath.c_str()));
			const FCookScopeCookMergeResult Merge = FCookScopeCookSnapshotReader::MergeDevelopmentRegistry(
				CookRegistryPath,
				UTF8_TO_TCHAR(Parsed.value.cookPlatform.c_str()),
				UTF8_TO_TCHAR(Parsed.value.cookConfiguration.c_str()),
				Candidate);
			if (!Merge.bSuccess)
			{
				UE_LOG(LogCookScopeCommandlet, Error, TEXT("Cook Registry merge failed: %s"), *Merge.Error);
				return cookscope::CommandletExitCode(cookscope::AuditStatus::InternalError);
			}
			Candidate = Merge.Snapshot;
		}

		std::optional<cookscope::Snapshot> Baseline;
		std::optional<cookscope::SnapshotDiffResult> Diff;
		if (!Parsed.value.baselinePath.empty())
		{
			const FString BaselinePath = FPaths::ConvertRelativePathToFull(UTF8_TO_TCHAR(Parsed.value.baselinePath.c_str()));
			std::string BaselineText;
			if (!LoadUtf8(BaselinePath, BaselineText))
			{
				UE_LOG(LogCookScopeCommandlet, Error, TEXT("Unable to read baseline: %s"), *BaselinePath);
				return cookscope::CommandletExitCode(cookscope::AuditStatus::InvalidInvocation);
			}
			const cookscope::SnapshotParseResult BaselineResult = cookscope::ParseSnapshot(BaselineText);
			if (!BaselineResult.ok)
			{
				UE_LOG(LogCookScopeCommandlet, Error, TEXT("Invalid baseline at %s: %s"),
					UTF8_TO_TCHAR(BaselineResult.error.path.c_str()), UTF8_TO_TCHAR(BaselineResult.error.message.c_str()));
				return cookscope::CommandletExitCode(cookscope::AuditStatus::InvalidInvocation);
			}
			Baseline = BaselineResult.value;
			Diff = cookscope::DiffSnapshots(*Baseline, Candidate);
			if (!Diff->comparable)
			{
				UE_LOG(LogCookScopeCommandlet, Error, TEXT("Baseline is not comparable: %s"), UTF8_TO_TCHAR(Diff->error.c_str()));
				return cookscope::CommandletExitCode(cookscope::AuditStatus::InvalidInvocation);
			}
		}

		const cookscope::AnalysisResult Analysis = cookscope::Evaluate(
			Candidate,
			Config.value,
			Baseline ? &*Baseline : nullptr);
		const cookscope::ReportSet Reports = cookscope::RenderReports(
			Candidate,
			Config.value,
			Analysis,
			Diff ? &*Diff : nullptr);

		if (FPlatformTime::Seconds() - StartedAt > Parsed.value.timeoutSeconds)
		{
			UE_LOG(LogCookScopeCommandlet, Error, TEXT("Audit exceeded timeout of %u seconds"), Parsed.value.timeoutSeconds);
			return cookscope::CommandletExitCode(cookscope::AuditStatus::Cancelled);
		}
		const FString OutputDirectory = FPaths::ConvertRelativePathToFull(UTF8_TO_TCHAR(Parsed.value.outputDirectory.c_str()));
		if (!IFileManager::Get().MakeDirectory(*OutputDirectory, true))
		{
			UE_LOG(LogCookScopeCommandlet, Error, TEXT("Unable to create output directory: %s"), *OutputDirectory);
			return cookscope::CommandletExitCode(cookscope::AuditStatus::InternalError);
		}
		const std::string SnapshotJson = cookscope::WriteCanonicalSnapshot(Candidate);
		if (!SaveUtf8(OutputDirectory / TEXT("cookscope.json"), Reports.json) ||
			!SaveUtf8(OutputDirectory / TEXT("cookscope.sarif"), Reports.sarif) ||
			!SaveUtf8(OutputDirectory / TEXT("cookscope.junit.xml"), Reports.junit) ||
			!SaveUtf8(OutputDirectory / TEXT("cookscope.html"), Reports.html) ||
			!SaveUtf8(OutputDirectory / TEXT("cookscope.snapshot.json"), SnapshotJson))
		{
			UE_LOG(LogCookScopeCommandlet, Error, TEXT("Unable to write one or more reports under %s"), *OutputDirectory);
			return cookscope::CommandletExitCode(cookscope::AuditStatus::InternalError);
		}

		const bool HasBlockingFinding = std::any_of(Analysis.findings.begin(), Analysis.findings.end(), [&](const cookscope::Finding& Finding) {
			return FindingBlocks(Finding, Config.value);
		});
		UE_LOG(LogCookScopeCommandlet, Display, TEXT("CookScope full audit wrote %d findings to %s"),
			static_cast<int32>(Analysis.findings.size()), *OutputDirectory);
		if (!Analysis.diagnostics.empty()) return cookscope::CommandletExitCode(cookscope::AuditStatus::InternalError);
		if (Parsed.value.failOnViolation && HasBlockingFinding) return cookscope::CommandletExitCode(cookscope::AuditStatus::Violation);
		return cookscope::CommandletExitCode(cookscope::AuditStatus::Clean);
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
	const bool FullAudit = std::any_of(storage.begin(), storage.end(), [](const std::string& Argument) {
		return Argument.starts_with("-config=") || Argument.starts_with("-source-sha=");
	});
	if (FullAudit)
	{
		return RunFullAudit(std::span<const std::string_view>(views));
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
