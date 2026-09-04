#pragma once

#include "CoreMinimal.h"
#include "cookscope/snapshot.h"

struct COOKSCOPEEDITOR_API FCookScopeScanResult
{
	bool bSuccess = false;
	bool bCancelled = false;
	FString Error;
	cookscope::Snapshot Snapshot;
};

struct COOKSCOPEEDITOR_API FCookScopeScanOptions
{
	bool bDiscoverOnDisk = true;
	bool bRefreshAssetManager = true;
	int32 MaximumAssets = 100000;
	int32 MaximumDependencies = 500000;
	TFunction<bool()> ShouldCancel;
};

class COOKSCOPEEDITOR_API FCookScopeAssetScanner
{
public:
	[[nodiscard]] static FCookScopeScanResult ScanPath(
		const FString& PackagePath,
		const FString& SourceSha,
		const FCookScopeScanOptions& Options = {});
};
