#include "CookScopeEditorSession.h"

#include "CookScopeAssetScanner.h"
#include "CookScopeCookSnapshotReader.h"

#include "cookscope/diff.h"
#include "cookscope/reports.h"
#include "cookscope/rule_config.h"
#include "cookscope/rules.h"
#include "cookscope/snapshot.h"

#include "Async/Async.h"
#include "HAL/CriticalSection.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

#include <algorithm>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	std::string EditorSessionToUtf8(const FString& Text)
	{
		FTCHARToUTF8 Converted(*Text);
		return std::string(Converted.Get(), static_cast<std::size_t>(Converted.Length()));
	}

	FString EditorSessionFromUtf8(std::string_view Text)
	{
		FUTF8ToTCHAR Converted(Text.data(), static_cast<int32>(Text.size()));
		return FString(Converted.Length(), Converted.Get());
	}

	FString EditorSeverity(cookscope::Severity Severity)
	{
		switch (Severity)
		{
		case cookscope::Severity::Note: return TEXT("note");
		case cookscope::Severity::Warning: return TEXT("warning");
		case cookscope::Severity::Error: return TEXT("error");
		}
		return TEXT("error");
	}

	bool SaveEditorReport(const FString& Path, const std::string& Text)
	{
		return FFileHelper::SaveStringToFile(
			EditorSessionFromUtf8(Text),
			*Path,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

class FCookScopeEditorSession::FImpl
{
public:
	mutable FCriticalSection Mutex;
	TFuture<void> Worker;
	ECookScopeEditorSessionState State = ECookScopeEditorSessionState::Idle;
	float Progress = 0.0f;
	FString Status = TEXT("Ready");
	bool CancelRequested = false;
	uint64 Generation = 0;
	cookscope::Snapshot Snapshot;
	cookscope::RuleConfig Config;
	cookscope::AnalysisResult Analysis;
	cookscope::DependencyGraph Graph;
	cookscope::ReportSet Reports;
};

FCookScopeEditorSession::FCookScopeEditorSession()
	: Impl(MakeUnique<FImpl>())
{
}

FCookScopeEditorSession::~FCookScopeEditorSession()
{
	Shutdown();
}

bool FCookScopeEditorSession::StartScan(const FCookScopeEditorScanSettings& Settings)
{
	if (Impl->Worker.IsValid()) Impl->Worker.Wait();
	{
		FScopeLock Lock(&Impl->Mutex);
		if (Impl->State == ECookScopeEditorSessionState::Scanning) return false;
		Impl->CancelRequested = false;
		Impl->State = ECookScopeEditorSessionState::Scanning;
		Impl->Progress = 0.05f;
		Impl->Status = TEXT("Loading rules");
		++Impl->Generation;
	}

	FString ConfigText;
	if (!FFileHelper::LoadFileToString(ConfigText, *Settings.ConfigPath))
	{
		FScopeLock Lock(&Impl->Mutex);
		Impl->State = ECookScopeEditorSessionState::Failed;
		Impl->Status = FString::Printf(TEXT("Unable to read config: %s"), *Settings.ConfigPath);
		return false;
	}
	const cookscope::RuleConfigParseResult Config = cookscope::ParseRuleConfig(EditorSessionToUtf8(ConfigText));
	if (!Config.ok)
	{
		FScopeLock Lock(&Impl->Mutex);
		Impl->State = ECookScopeEditorSessionState::Failed;
		Impl->Status = FString::Printf(TEXT("Invalid rules at %s"), *EditorSessionFromUtf8(Config.error.path));
		return false;
	}

	{
		FScopeLock Lock(&Impl->Mutex);
		Impl->Progress = 0.2f;
		Impl->Status = TEXT("Scanning Asset Registry");
	}
	FCookScopeScanResult Scan = FCookScopeAssetScanner::ScanPath(Settings.Scope, Settings.SourceSha);
	if (!Scan.bSuccess)
	{
		FScopeLock Lock(&Impl->Mutex);
		Impl->State = ECookScopeEditorSessionState::Failed;
		Impl->Status = Scan.Error;
		return false;
	}
	if (!Settings.CookRegistryPath.IsEmpty())
	{
		const FCookScopeCookMergeResult Merge = FCookScopeCookSnapshotReader::MergeDevelopmentRegistry(
			Settings.CookRegistryPath,
			Settings.CookPlatform,
			Settings.CookConfiguration,
			Scan.Snapshot);
		if (!Merge.bSuccess)
		{
			FScopeLock Lock(&Impl->Mutex);
			Impl->State = ECookScopeEditorSessionState::Failed;
			Impl->Status = Merge.Error;
			return false;
		}
		Scan.Snapshot = Merge.Snapshot;
	}

	std::optional<cookscope::Snapshot> Baseline;
	if (!Settings.BaselinePath.IsEmpty())
	{
		FString BaselineText;
		if (!FFileHelper::LoadFileToString(BaselineText, *Settings.BaselinePath))
		{
			FScopeLock Lock(&Impl->Mutex);
			Impl->State = ECookScopeEditorSessionState::Failed;
			Impl->Status = TEXT("Unable to read baseline");
			return false;
		}
		const cookscope::SnapshotParseResult ParsedBaseline = cookscope::ParseSnapshot(EditorSessionToUtf8(BaselineText));
		if (!ParsedBaseline.ok)
		{
			FScopeLock Lock(&Impl->Mutex);
			Impl->State = ECookScopeEditorSessionState::Failed;
			Impl->Status = TEXT("Baseline is invalid");
			return false;
		}
		Baseline = ParsedBaseline.value;
	}

	const uint64 Generation = Impl->Generation;
	cookscope::Snapshot Snapshot = std::move(Scan.Snapshot);
	cookscope::RuleConfig Rules = Config.value;
	Impl->Worker = Async(EAsyncExecution::ThreadPool, [this, Generation, Snapshot = std::move(Snapshot), Rules = std::move(Rules), Baseline = std::move(Baseline)]() mutable {
		{
			FScopeLock Lock(&Impl->Mutex);
			if (Impl->CancelRequested || Generation != Impl->Generation) return;
			Impl->Progress = 0.55f;
			Impl->Status = TEXT("Evaluating shared rules");
		}
		std::optional<cookscope::SnapshotDiffResult> Diff;
		if (Baseline)
		{
			Diff = cookscope::DiffSnapshots(*Baseline, Snapshot);
			if (!Diff->comparable)
			{
				FScopeLock Lock(&Impl->Mutex);
				if (!Impl->CancelRequested && Generation == Impl->Generation)
				{
					Impl->State = ECookScopeEditorSessionState::Failed;
					Impl->Status = EditorSessionFromUtf8(Diff->error);
				}
				return;
			}
		}
		cookscope::AnalysisResult Analysis = cookscope::Evaluate(Snapshot, Rules, Baseline ? &*Baseline : nullptr);
		if (Baseline && Diff)
		{
			const cookscope::AnalysisResult Before = cookscope::Evaluate(*Baseline, Rules);
			const cookscope::AnalysisResult After = cookscope::Evaluate(Snapshot, Rules);
			cookscope::AppendFindingChanges(Before, After, *Diff);
		}
		const cookscope::GraphBuildResult Graph = cookscope::BuildDependencyGraph(Snapshot, cookscope::OperationLimits{});
		if (Graph.state != cookscope::OperationState::Complete)
		{
			FScopeLock Lock(&Impl->Mutex);
			if (!Impl->CancelRequested && Generation == Impl->Generation)
			{
				Impl->State = ECookScopeEditorSessionState::Failed;
				Impl->Status = TEXT("Dependency graph exceeded limits");
			}
			return;
		}
		cookscope::ReportSet Reports = cookscope::RenderReports(Snapshot, Rules, Analysis, Diff ? &*Diff : nullptr);
		FScopeLock Lock(&Impl->Mutex);
		if (Impl->CancelRequested || Generation != Impl->Generation) return;
		Impl->Snapshot = std::move(Snapshot);
		Impl->Config = std::move(Rules);
		Impl->Analysis = std::move(Analysis);
		Impl->Graph = Graph.graph;
		Impl->Reports = std::move(Reports);
		Impl->Progress = 1.0f;
		Impl->Status = TEXT("Scan complete");
		Impl->State = ECookScopeEditorSessionState::Complete;
	});
	return true;
}

void FCookScopeEditorSession::Cancel()
{
	FScopeLock Lock(&Impl->Mutex);
	Impl->CancelRequested = true;
	++Impl->Generation;
	if (Impl->State == ECookScopeEditorSessionState::Scanning)
	{
		Impl->State = ECookScopeEditorSessionState::Cancelled;
		Impl->Status = TEXT("Scan cancelled");
	}
}

void FCookScopeEditorSession::Shutdown()
{
	Cancel();
	if (Impl->Worker.IsValid()) Impl->Worker.Wait();
	FScopeLock Lock(&Impl->Mutex);
	Impl->Snapshot = {};
	Impl->Config = {};
	Impl->Analysis = {};
	Impl->Graph = {};
	Impl->Reports = {};
	Impl->Progress = 0.0f;
	Impl->Status = TEXT("Ready");
	Impl->State = ECookScopeEditorSessionState::Idle;
}

ECookScopeEditorSessionState FCookScopeEditorSession::GetState() const
{
	FScopeLock Lock(&Impl->Mutex);
	return Impl->State;
}

float FCookScopeEditorSession::GetProgress() const
{
	FScopeLock Lock(&Impl->Mutex);
	return Impl->Progress;
}

FString FCookScopeEditorSession::GetStatusText() const
{
	FScopeLock Lock(&Impl->Mutex);
	return Impl->Status;
}

TArray<FCookScopeEditorFindingItem> FCookScopeEditorSession::GetFilteredFindings(const FCookScopeEditorFilter& Filter) const
{
	FScopeLock Lock(&Impl->Mutex);
	TArray<FCookScopeEditorFindingItem> Items;
	for (const cookscope::Finding& Finding : Impl->Analysis.findings)
	{
		FString AssetClass;
		const auto Asset = std::find_if(Impl->Snapshot.assets.begin(), Impl->Snapshot.assets.end(), [&](const cookscope::AssetRecord& Item) {
			return Item.objectPath == Finding.assetPath;
		});
		if (Asset != Impl->Snapshot.assets.end()) AssetClass = EditorSessionFromUtf8(Asset->assetClass);
		const FString Severity = EditorSeverity(Finding.severity);
		const FString RuleId = EditorSessionFromUtf8(Finding.ruleId);
		const FString AssetPath = EditorSessionFromUtf8(Finding.assetPath);
		if (!Filter.Severity.IsEmpty() && !Severity.Equals(Filter.Severity, ESearchCase::IgnoreCase)) continue;
		if (!Filter.RuleId.IsEmpty() && !RuleId.Contains(Filter.RuleId, ESearchCase::IgnoreCase)) continue;
		if (!Filter.AssetClass.IsEmpty() && !AssetClass.Equals(Filter.AssetClass, ESearchCase::IgnoreCase)) continue;
		if (!Filter.PathSearch.IsEmpty() && !AssetPath.Contains(Filter.PathSearch, ESearchCase::IgnoreCase)) continue;
		Items.Add({Severity, RuleId, AssetPath, AssetClass, EditorSessionFromUtf8(Finding.message)});
	}
	return Items;
}

cookscope::WhyCookedResult FCookScopeEditorSession::ExplainWhyCooked(
	const TArray<FString>& Roots,
	const FString& Target) const
{
	FScopeLock Lock(&Impl->Mutex);
	std::vector<std::string> Storage;
	Storage.reserve(Roots.Num());
	for (const FString& Root : Roots) Storage.push_back(EditorSessionToUtf8(Root));
	std::vector<std::string_view> Views;
	Views.reserve(Storage.size());
	for (const std::string& Root : Storage) Views.emplace_back(Root);
	return cookscope::ExplainWhyCooked(
		Impl->Graph,
		std::span<const std::string_view>(Views),
		EditorSessionToUtf8(Target),
		cookscope::DependencyMask::All(),
		cookscope::OperationLimits{});
}

FString FCookScopeEditorSession::DescribeAsset(const FString& AssetPath) const
{
	FScopeLock Lock(&Impl->Mutex);
	const std::string Path = EditorSessionToUtf8(AssetPath);
	const auto Asset = std::find_if(Impl->Snapshot.assets.begin(), Impl->Snapshot.assets.end(), [&](const cookscope::AssetRecord& Item) {
		return Item.objectPath == Path;
	});
	if (Asset == Impl->Snapshot.assets.end()) return TEXT("Asset not found");
	FString Details = FString::Printf(TEXT("%s\nClass %s"), *AssetPath, *EditorSessionFromUtf8(Asset->assetClass));
	if (Asset->primaryAssetId) Details += FString::Printf(TEXT("\nPrimary Asset %s"), *EditorSessionFromUtf8(*Asset->primaryAssetId));
	for (const int32 Chunk : Asset->chunkIds) Details += FString::Printf(TEXT("\nChunk %d"), Chunk);
	for (const std::string& Bundle : Asset->assetBundles) Details += FString::Printf(TEXT("\nBundle %s"), *EditorSessionFromUtf8(Bundle));
	if (Asset->cookedSize.kind == cookscope::MeasurementKind::ActualCooked && Asset->cookedSize.bytes)
	{
		Details += FString::Printf(TEXT("\nActual Cook %llu bytes"), static_cast<unsigned long long>(*Asset->cookedSize.bytes));
	}
	else
	{
		Details += TEXT("\nActual Cook unavailable");
	}
	Details += FString::Printf(TEXT("\nDependencies %d"), static_cast<int32>(Asset->dependencies.size()));
	return Details;
}

bool FCookScopeEditorSession::ExportReports(const FString& OutputDirectory) const
{
	FScopeLock Lock(&Impl->Mutex);
	if (Impl->State != ECookScopeEditorSessionState::Complete || !IFileManager::Get().MakeDirectory(*OutputDirectory, true)) return false;
	return SaveEditorReport(OutputDirectory / TEXT("cookscope.json"), Impl->Reports.json) &&
		SaveEditorReport(OutputDirectory / TEXT("cookscope.sarif"), Impl->Reports.sarif) &&
		SaveEditorReport(OutputDirectory / TEXT("cookscope.junit.xml"), Impl->Reports.junit) &&
		SaveEditorReport(OutputDirectory / TEXT("cookscope.html"), Impl->Reports.html);
}

