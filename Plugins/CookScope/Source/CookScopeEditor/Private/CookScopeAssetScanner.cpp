#include "CookScopeAssetScanner.h"

#include "AssetRegistry/AssetBundleData.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetIdentifier.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformProperties.h"
#include "Misc/EngineVersion.h"
#include "Sound/SoundWave.h"

#include <algorithm>
#include <string>

namespace
{
	std::string ScannerToUtf8(const FString& Text)
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
			if (Path.IsValid()) return ScannerToUtf8(Path.ToString());
		}
		if (!Identifier.PackageName.IsNone() && !Identifier.IsValue())
		{
			TArray<FAssetData> Assets;
			Registry.GetAssetsByPackageName(Identifier.PackageName, Assets, true);
			Assets.Sort([](const FAssetData& Left, const FAssetData& Right) {
				return Left.GetSoftObjectPath().ToString() < Right.GetSoftObjectPath().ToString();
			});
			if (!Assets.IsEmpty()) return ScannerToUtf8(Assets[0].GetSoftObjectPath().ToString());
		}
		return ScannerToUtf8(Identifier.ToString());
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
		Output.primaryAssetId = ScannerToUtf8(PrimaryId.ToString());

		const FPrimaryAssetRules Rules = AssetManager.GetPrimaryAssetRules(PrimaryId);
		if (Rules.ChunkId >= 0) Output.chunkIds.push_back(Rules.ChunkId);

		TArray<FAssetBundleEntry> Entries;
		if (AssetManager.GetAssetBundleEntries(PrimaryId, Entries))
		{
			for (const FAssetBundleEntry& Entry : Entries)
			{
				Output.assetBundles.push_back(ScannerToUtf8(Entry.BundleName.ToString()));
				for (const FTopLevelAssetPath& Path : Entry.AssetPaths)
				{
					Output.dependencies.push_back({ScannerToUtf8(Path.ToString()), cookscope::DependencyKind::Manage});
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

	void SetNormalizedTag(cookscope::AssetRecord& Record, const char* Name, const FString& Value)
	{
		Record.tags.insert_or_assign(Name, ScannerToUtf8(Value));
	}

	void CopyNormalizedTag(
		const FAssetData& Asset,
		const FName SourceName,
		cookscope::AssetRecord& Record,
		const char* TargetName)
	{
		FString Value;
		if (Asset.GetTagValue(SourceName, Value) && !Value.IsEmpty()) SetNormalizedTag(Record, TargetName, Value);
	}

	void AddResourceMetadata(const FAssetData& Asset, cookscope::AssetRecord& Record)
	{
		if (Asset.AssetClassPath == UTexture2D::StaticClass()->GetClassPathName())
		{
			if (const UTexture2D* Texture = Cast<UTexture2D>(Asset.GetAsset()))
			{
				const FIntPoint Size = Texture->GetImportedSize();
				if (Size.X > 0) SetNormalizedTag(Record, "TextureWidth", FString::FromInt(Size.X));
				if (Size.Y > 0) SetNormalizedTag(Record, "TextureHeight", FString::FromInt(Size.Y));
				const int32 MipCount = FMath::Max(Texture->GetNumMips(), Texture->Source.GetNumMips());
				if (MipCount > 0) SetNormalizedTag(Record, "TextureMips", FString::FromInt(MipCount));
			}
			CopyNormalizedTag(Asset, TEXT("Format"), Record, "TextureFormat");
		}
		else if (Asset.AssetClassPath == UStaticMesh::StaticClass()->GetClassPathName())
		{
			CopyNormalizedTag(Asset, TEXT("Triangles"), Record, "MeshTriangles");
		}
		else if (Asset.AssetClassPath == USkeletalMesh::StaticClass()->GetClassPathName())
		{
			CopyNormalizedTag(Asset, TEXT("Vertices"), Record, "MeshVertices");
		}
		else if (Asset.AssetClassPath == USoundWave::StaticClass()->GetClassPathName())
		{
			if (const USoundWave* Sound = Cast<USoundWave>(Asset.GetAsset()))
			{
				const float DurationSeconds = Sound->GetDuration();
				if (FMath::IsFinite(DurationSeconds) && DurationSeconds > 0.0f)
				{
					SetNormalizedTag(Record, "SoundDurationMs", FString::Printf(TEXT("%lld"), FMath::RoundToInt64(DurationSeconds * 1000.0)));
				}
				FString Format = Sound->GetRuntimeFormat().ToString();
				if (Format.IsEmpty() || Format == TEXT("None"))
				{
					Format = StaticEnum<ESoundAssetCompressionType>()->GetNameStringByValue(
						static_cast<int64>(Sound->GetSoundAssetCompressionType()));
				}
				if (!Format.IsEmpty()) SetNormalizedTag(Record, "SoundFormat", Format);
			}
		}
	}
}

FCookScopeScanResult FCookScopeAssetScanner::ScanPath(
	const FString& PackagePath,
	const FString& SourceSha,
	const FCookScopeScanOptions& Options)
{
	FCookScopeScanResult Result;
	if (PackagePath.IsEmpty())
	{
		Result.Error = TEXT("Package path must not be empty");
		return Result;
	}
	if (Options.MaximumAssets <= 0 || Options.MaximumDependencies <= 0)
	{
		Result.Error = TEXT("Asset and dependency limits must be positive");
		return Result;
	}
	auto Cancelled = [&]() {
		if (!Options.ShouldCancel || !Options.ShouldCancel()) return false;
		Result.bCancelled = true;
		Result.Error = TEXT("Asset Registry scan cancelled or timed out");
		return true;
	};
	if (Cancelled()) return Result;

	FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();
	if (Options.bDiscoverOnDisk) Registry.ScanPathsSynchronous({PackagePath}, true);
	UAssetManager& AssetManager = UAssetManager::Get();
	if (Options.bRefreshAssetManager)
	{
		AssetManager.UpdateManagementDatabase(EUpdateManagementDatabaseFlags::BuildChunkMap | EUpdateManagementDatabaseFlags::ForceRefresh);
	}

	TArray<FAssetData> Assets;
	if (!Registry.GetAssetsByPath(FName(*PackagePath), Assets, true, true))
	{
		Result.Error = FString::Printf(TEXT("Asset Registry could not scan path: %s"), *PackagePath);
		return Result;
	}
	if (Cancelled()) return Result;
	if (Assets.Num() > Options.MaximumAssets)
	{
		Result.Error = FString::Printf(
			TEXT("Asset Registry result exceeded asset limit %d for %s"),
			Options.MaximumAssets,
			*PackagePath);
		return Result;
	}
	Assets.Sort([](const FAssetData& Left, const FAssetData& Right) {
		return Left.GetSoftObjectPath().ToString() < Right.GetSoftObjectPath().ToString();
	});

	cookscope::Snapshot Snapshot;
	Snapshot.provenance.engineVersion = ScannerToUtf8(FEngineVersion::Current().ToString());
	Snapshot.provenance.platform = ScannerToUtf8(FPlatformProperties::IniPlatformName());
	Snapshot.provenance.cookConfiguration = "EditorAssetRegistry";
	Snapshot.provenance.sourceSha = ScannerToUtf8(SourceSha);
	int64 DependencyCount = 0;
	for (const FAssetData& Asset : Assets)
	{
		if (Cancelled()) return Result;
		cookscope::AssetRecord Record;
		Record.objectPath = ScannerToUtf8(Asset.GetSoftObjectPath().ToString());
		Record.packageName = ScannerToUtf8(Asset.PackageName.ToString());
		Record.assetClass = ScannerToUtf8(Asset.AssetClassPath.ToString());
		Record.packagePath = ScannerToUtf8(Asset.PackagePath.ToString());
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
			Record.tags.emplace(ScannerToUtf8(Pair.Key.ToString()), ScannerToUtf8(Pair.Value.GetStorageString()));
		});
		if (Asset.TaggedAssetBundles)
		{
			for (const FAssetBundleEntry& Entry : Asset.TaggedAssetBundles->Bundles)
			{
				Record.assetBundles.push_back(ScannerToUtf8(Entry.BundleName.ToString()));
				for (const FTopLevelAssetPath& Path : Entry.AssetPaths)
				{
					Record.dependencies.push_back({ScannerToUtf8(Path.ToString()), cookscope::DependencyKind::Manage});
				}
			}
		}
		AddResourceMetadata(Asset, Record);
		Record.sourceProvenance = "ue-asset-registry";
		AddRegistryDependencies(FAssetIdentifier(Asset.PackageName), Registry, AssetManager, Record.dependencies);
		AddManagerData(AssetManager.GetPrimaryAssetIdForData(Asset), Registry, AssetManager, Record);
		std::sort(Record.chunkIds.begin(), Record.chunkIds.end());
		Record.chunkIds.erase(std::unique(Record.chunkIds.begin(), Record.chunkIds.end()), Record.chunkIds.end());
		std::sort(Record.assetBundles.begin(), Record.assetBundles.end());
		Record.assetBundles.erase(std::unique(Record.assetBundles.begin(), Record.assetBundles.end()), Record.assetBundles.end());
		std::sort(Record.dependencies.begin(), Record.dependencies.end(), [](const cookscope::DependencyEdge& Left, const cookscope::DependencyEdge& Right) {
			if (Left.target != Right.target) return Left.target < Right.target;
			return static_cast<uint8>(Left.kind) < static_cast<uint8>(Right.kind);
		});
		Record.dependencies.erase(
			std::unique(Record.dependencies.begin(), Record.dependencies.end(), [](const cookscope::DependencyEdge& Left, const cookscope::DependencyEdge& Right) {
				return Left.target == Right.target && Left.kind == Right.kind;
			}),
			Record.dependencies.end());
		Record.dependencies.erase(
			std::remove_if(Record.dependencies.begin(), Record.dependencies.end(), [&](const cookscope::DependencyEdge& Dependency) {
				return Dependency.kind == cookscope::DependencyKind::Manage && Dependency.target == Record.objectPath;
			}),
			Record.dependencies.end());
		DependencyCount += static_cast<int64>(Record.dependencies.size());
		if (DependencyCount > Options.MaximumDependencies)
		{
			Result.Error = FString::Printf(
				TEXT("Asset Registry result exceeded dependency limit %d for %s"),
				Options.MaximumDependencies,
				*PackagePath);
			return Result;
		}
		Snapshot.assets.push_back(std::move(Record));
	}
	if (Cancelled()) return Result;

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
