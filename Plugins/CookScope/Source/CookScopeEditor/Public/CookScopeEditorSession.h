#pragma once

#include "CoreMinimal.h"

#include "cookscope/graph.h"

enum class ECookScopeEditorSessionState : uint8
{
	Idle,
	Scanning,
	Complete,
	Failed,
	Cancelled,
};

struct COOKSCOPEEDITOR_API FCookScopeEditorScanSettings
{
	FString Scope = TEXT("/Game");
	FString ConfigPath;
	FString SourceSha;
	FString BaselinePath;
	FString CookRegistryPath;
	FString CookPlatform = TEXT("Windows");
	FString CookConfiguration = TEXT("Development");
};

struct COOKSCOPEEDITOR_API FCookScopeEditorFilter
{
	FString Severity;
	FString RuleId;
	FString AssetClass;
	FString PathSearch;
};

struct COOKSCOPEEDITOR_API FCookScopeEditorFindingItem
{
	FString Severity;
	FString RuleId;
	FString AssetPath;
	FString AssetClass;
	FString Message;
};

class COOKSCOPEEDITOR_API FCookScopeEditorSession
{
public:
	FCookScopeEditorSession();
	~FCookScopeEditorSession();

	FCookScopeEditorSession(const FCookScopeEditorSession&) = delete;
	FCookScopeEditorSession& operator=(const FCookScopeEditorSession&) = delete;

	bool StartScan(const FCookScopeEditorScanSettings& Settings);
	void Cancel();
	void Shutdown();

	[[nodiscard]] ECookScopeEditorSessionState GetState() const;
	[[nodiscard]] float GetProgress() const;
	[[nodiscard]] FString GetStatusText() const;
	[[nodiscard]] TArray<FCookScopeEditorFindingItem> GetFilteredFindings(const FCookScopeEditorFilter& Filter) const;
	[[nodiscard]] cookscope::WhyCookedResult ExplainWhyCooked(
		const TArray<FString>& Roots,
		const FString& Target) const;
	[[nodiscard]] FString DescribeFinding(const FCookScopeEditorFindingItem& Item) const;
	[[nodiscard]] FString DescribeAsset(const FString& AssetPath) const;
	[[nodiscard]] FString GetComparisonSummary() const;
	[[nodiscard]] bool ExportReports(const FString& OutputDirectory) const;

private:
	class FImpl;
	TUniquePtr<FImpl> Impl;
};
