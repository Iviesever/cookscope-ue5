#pragma once

#include "CoreMinimal.h"
#include "cookscope/snapshot.h"

struct COOKSCOPEEDITOR_API FCookScopeScanResult
{
	bool bSuccess = false;
	FString Error;
	cookscope::Snapshot Snapshot;
};

class COOKSCOPEEDITOR_API FCookScopeAssetScanner
{
public:
	[[nodiscard]] static FCookScopeScanResult ScanPath(const FString& PackagePath, const FString& SourceSha);
};

