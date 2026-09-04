#include "CookScopeAssetScanner.h"

#include "AssetRegistry/AssetBundleData.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetIdentifier.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "HAL/PlatformProperties.h"
#include "Misc/EngineVersion.h"

#include <algorithm>
#include <string>

namespace
{
	std::string ToUtf8(const FString& Text)
	{
		FTCHARToUTF8 Converted(*Text);
		return std::string(Converted.Get(), static_cast<std::size_t>(Converted.Length()));
	}

	std::string ResolveIdentifier(
		const FAssetIdentifier& Identifier,
		IAssetRegistry& Registry,
		UAssetManager& AssetManager)
	{
		if (const FPrimaryAssetId PrimaryId = Identifier.GetPrimaryAssetId(); PrimaryId.IsValid())
		{
			const FSoftObjectPath Path = AssetManager.GetPrimaryAssetPath(PrimaryId);
			if (Path.IsValid()) return ToUtf8(Path.ToString());
		}
		if (!Identifier.PackageName.IsNone() && !Identifier.IsValue())
		{
			TArray<FAssetData> Assets;
			Registry.GetAssetsByPackageName(Identifier.PackageName, Assets, true);
			Assets.Sort([](const FAssetData& Left, const FAssetData& Right) {
				return Left.GetSoftObjectPath().ToString() < Right.GetSoftObjectPath().ToString();
			});
			if (!Assets.IsEmpty()) return ToUtf8(Assets[0].GetSoftObjectPath().ToString());
		}
		return ToUtf8(Identifier.ToString());
	}

	cookscope::DependencyKind DependencyKindFor(const FAssetDependency& Dependency)
	{
		using namespace UE::AssetRegistry;
		if (EnumHasAnyFlags(Dependency.Category, EDependencyCategory::Manage))
		{
			return cookscope::DependencyKind::Manage;
		}
		if (EnumHasAnyFlags(Dependency.Category, EDependencyCategory::SearchableName))
		{
			return cookscope::DependencyKind::SearchableName;
		}
		return EnumHasAnyFlags(Dependency.Properties, EDependencyProperty::Hard)
			? cookscope::DependencyKind::Hard
			: cookscope::DependencyKind::Soft;
	}

	void AddRegistryDependencies(
		const FAssetIdentifier& Source,
		IAssetRegistry& Registry,
		UAssetManager& AssetManager,
		std::vector<cookscope::DependencyEdge>& Output)
	{
		TArray<FAssetDependency> Dependencies;
		Registry.GetDependencies(Source, Dependencies, UE::AssetRegistry::EDependencyCategory::All);
		for (const FAssetDependency& Dependency : Dependencies)
		{
			Output.push_back({ResolveIdentifier(Dependency.AssetId, Registry, AssetManager), DependencyKindFor(Dependency)});
		}
	}

	void AddManagerData(
		const FPrimaryAssetId& PrimaryId,
		IAssetRegistry& Registry,
		UAssetManager& AssetManager,
		cookscope::AssetRecord& Output)
	{
		if (!PrimaryId.IsValid()) return;
		Output.primaryAssetId = ToUtf8(PrimaryId.ToString());

		const FPrimaryAssetRules Rules = AssetManager.GetPrimaryAssetRules(PrimaryId);
		if (Rules.ChunkId >= 0) Output.chunkIds.push_back(Rules.ChunkId);

		TArray<FAssetBundleEntry> Entries;
		if (AssetManager.GetAssetBundleEntries(PrimaryId, Entries))
		{
			for (const FAssetBundleEntry& Entry : Entries)
			{
				Output.assetBundles.push_back(ToUtf8(Entry.BundleName.ToString()));
				for (const FTopLevelAssetPath& Path : Entry.AssetPaths)
				{
					Output.dependencies.push_back({ToUtf8(Path.ToString()), cookscope::DependencyKind::Manage});
				}
			}
		}

		TArray<FName> ManagedPackages;
		if (AssetManager.GetManagedPackageList(PrimaryId, ManagedPackages))
		{
			for (const FName PackageName : ManagedPackages)
			{
				Output.dependencies.push_back({
					ResolveIdentifier(FAssetIdentifier(PackageName), Registry, AssetManager),
					cookscope::DependencyKind::Manage});
			}
		}
		AddRegistryDependencies(FAssetIdentifier(PrimaryId), Registry, AssetManager, Output.dependencies);
	}
}

FCookScopeScanResult FCookScopeAssetScanner::ScanPath(const FString& PackagePath, const FString& SourceSha)
{
	FCookScopeScanResult Result;
	if (PackagePath.IsEmpty())
	{
		Result.Error = TEXT("Package path must not be empty");
		return Result;
	}

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();
	Registry.ScanPathsSynchronous({PackagePath}, true);
	UAssetManager& AssetManager = UAssetManager::Get();
	AssetManager.UpdateManagementDatabase(EUpdateManagementDatabaseFlags::BuildChunkMap | EUpdateManagementDatabaseFlags::ForceRefresh);

	TArray<FAssetData> Assets;
	if (!Registry.GetAssetsByPath(FName(*PackagePath), Assets, true, true))
	{
		Result.Error = FString::Printf(TEXT("Asset Registry could not scan path: %s"), *PackagePath);
		return Result;
	}
	Assets.Sort([](const FAssetData& Left, const FAssetData& Right) {
		return Left.GetSoftObjectPath().ToString() < Right.GetSoftObjectPath().ToString();
	});

	cookscope::Snapshot Snapshot;
	Snapshot.provenance.engineVersion = ToUtf8(FEngineVersion::Current().ToString());
	Snapshot.provenance.platform = ToUtf8(FPlatformProperties::IniPlatformName());
	Snapshot.provenance.cookConfiguration = "EditorAssetRegistry";
	Snapshot.provenance.sourceSha = ToUtf8(SourceSha);
	for (const FAssetData& Asset : Assets)
	{
		cookscope::AssetRecord Record;
		Record.objectPath = ToUtf8(Asset.GetSoftObjectPath().ToString());
		Record.packageName = ToUtf8(Asset.PackageName.ToString());
		Record.assetClass = ToUtf8(Asset.AssetClassPath.ToString());
		Record.packagePath = ToUtf8(Asset.PackagePath.ToString());
		Record.diskSize.kind = cookscope::MeasurementKind::Unavailable;
		FAssetPackageData PackageData;
		if (Registry.TryGetAssetPackageData(Asset.PackageName, PackageData) == UE::AssetRegistry::EExists::Exists && PackageData.DiskSize >= 0)
		{
			Record.diskSize.kind = cookscope::MeasurementKind::PackageDisk;
			Record.diskSize.bytes = static_cast<std::uint64_t>(PackageData.DiskSize);
		}
		Record.cookedSize.kind = cookscope::MeasurementKind::Unavailable;
		for (const int32 ChunkId : Asset.GetChunkIDs()) Record.chunkIds.push_back(ChunkId);
		Asset.TagsAndValues.ForEach([&](const TPair<FName, FAssetTagValueRef>& Pair) {
			Record.tags.emplace(ToUtf8(Pair.Key.ToString()), ToUtf8(Pair.Value.GetStorageString()));
		});
		if (Asset.TaggedAssetBundles)
		{
			for (const FAssetBundleEntry& Entry : Asset.TaggedAssetBundles->Bundles)
			{
				Record.assetBundles.push_back(ToUtf8(Entry.BundleName.ToString()));
			}
		}
		Record.sourceProvenance = "ue-asset-registry";
		AddRegistryDependencies(FAssetIdentifier(Asset.PackageName), Registry, AssetManager, Record.dependencies);
		AddManagerData(AssetManager.GetPrimaryAssetIdForData(Asset), Registry, AssetManager, Record);
		Snapshot.assets.push_back(std::move(Record));
	}

	const cookscope::SnapshotParseResult Normalized = cookscope::ParseSnapshot(cookscope::WriteCanonicalSnapshot(Snapshot));
	if (!Normalized.ok)
	{
		Result.Error = FString::Printf(
			TEXT("Core rejected UE snapshot at %s: %s"),
			UTF8_TO_TCHAR(Normalized.error.path.c_str()),
			UTF8_TO_TCHAR(Normalized.error.message.c_str()));
		return Result;
	}
	Result.bSuccess = true;
	Result.Snapshot = Normalized.value;
	return Result;
}

