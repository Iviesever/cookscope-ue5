#include "CookScopeCookSnapshotReader.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "Misc/Paths.h"

#include <string>

namespace
{
	std::string CookReaderToUtf8(const FString& Text)
	{
		FTCHARToUTF8 Converted(*Text);
		return std::string(Converted.Get(), static_cast<std::size_t>(Converted.Length()));
	}
}

FCookScopeCookMergeResult FCookScopeCookSnapshotReader::MergeDevelopmentRegistry(
	const FString& DevelopmentRegistryPath,
	const FString& Platform,
	const FString& CookConfiguration,
	const cookscope::Snapshot& SourceSnapshot)
{
	FCookScopeCookMergeResult Result;
	FAssetRegistryState DevelopmentState;
	if (!FAssetRegistryState::LoadFromDisk(
		*DevelopmentRegistryPath,
		FAssetRegistryLoadOptions(),
		DevelopmentState))
	{
		Result.Error = FString::Printf(TEXT("Unable to load Development Asset Registry: %s"), *DevelopmentRegistryPath);
		return Result;
	}

	const FString RuntimeRegistryPath = FPaths::Combine(
		FPaths::GetPath(FPaths::GetPath(DevelopmentRegistryPath)),
		TEXT("AssetRegistry.bin"));
	FAssetRegistryState RuntimeState;
	if (!FAssetRegistryState::LoadFromDisk(
		*RuntimeRegistryPath,
		FAssetRegistryLoadOptions(),
		RuntimeState))
	{
		Result.Error = FString::Printf(TEXT("Unable to load runtime Asset Registry: %s"), *RuntimeRegistryPath);
		return Result;
	}

	TArray<FName> RuntimePackages;
	RuntimeState.GetPackageNames(RuntimePackages);
	TSet<FName> RuntimePackageSet;
	RuntimePackageSet.Reserve(RuntimePackages.Num());
	for (const FName PackageName : RuntimePackages) RuntimePackageSet.Add(PackageName);
	cookscope::Snapshot Merged = SourceSnapshot;
	Merged.provenance.platform = CookReaderToUtf8(Platform);
	Merged.provenance.cookConfiguration = CookReaderToUtf8(CookConfiguration);
	for (cookscope::AssetRecord& Asset : Merged.assets)
	{
		const FName PackageName(UTF8_TO_TCHAR(Asset.packageName.c_str()));
		Asset.cookedSize = {cookscope::MeasurementKind::Unavailable, std::nullopt};
		if (!RuntimePackageSet.Contains(PackageName)) continue;
		const FAssetPackageData* PackageData = DevelopmentState.GetAssetPackageData(PackageName);
		if (!PackageData || PackageData->DiskSize < 0) continue;
		Asset.cookedSize.kind = cookscope::MeasurementKind::ActualCooked;
		Asset.cookedSize.bytes = static_cast<std::uint64_t>(PackageData->DiskSize);
	}

	const cookscope::SnapshotParseResult Normalized = cookscope::ParseSnapshot(cookscope::WriteCanonicalSnapshot(Merged));
	if (!Normalized.ok)
	{
		Result.Error = FString::Printf(
			TEXT("Core rejected merged Cook snapshot at %s: %s"),
			UTF8_TO_TCHAR(Normalized.error.path.c_str()),
			UTF8_TO_TCHAR(Normalized.error.message.c_str()));
		return Result;
	}
	Result.bSuccess = true;
	Result.Snapshot = Normalized.value;
	return Result;
}
