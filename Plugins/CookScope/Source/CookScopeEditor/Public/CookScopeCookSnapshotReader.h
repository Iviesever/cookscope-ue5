#pragma once

#include "CoreMinimal.h"
#include "cookscope/snapshot.h"

struct COOKSCOPEEDITOR_API FCookScopeCookMergeResult
{
	bool bSuccess = false;
	FString Error;
	cookscope::Snapshot Snapshot;
};

class COOKSCOPEEDITOR_API FCookScopeCookSnapshotReader
{
public:
	[[nodiscard]] static FCookScopeCookMergeResult MergeDevelopmentRegistry(
		const FString& DevelopmentRegistryPath,
		const FString& Platform,
		const FString& CookConfiguration,
		const cookscope::Snapshot& SourceSnapshot);
};

