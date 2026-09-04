#include "CookScopeFixtureBuilderCommandlet.h"

#include "CookScopeFixtureAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogCookScopeFixtureBuilder, Log, All);

namespace
{
	constexpr TCHAR FixtureRoot[] = TEXT("/Game/CookScopeFixtures");
	constexpr TCHAR ResourceFixtureRoot[] = TEXT("/Game/CookScopeResourceFixtures");

	struct FFixture
	{
		FString PackageName;
		UCookScopeFixtureAsset* Asset = nullptr;
	};

	struct FObjectFixture
	{
		FString PackageName;
		UObject* Asset = nullptr;
		bool bCreated = false;
	};

	FFixture FindOrCreateFixture(const TCHAR* RelativePackage, const TCHAR* AssetName)
	{
		const FString PackageName = FString(FixtureRoot) / RelativePackage;
		const FString ObjectPath = PackageName + TEXT(".") + AssetName;
		if (UCookScopeFixtureAsset* Existing = LoadObject<UCookScopeFixtureAsset>(nullptr, *ObjectPath))
		{
			return {PackageName, Existing};
		}

		UPackage* Package = CreatePackage(*PackageName);
		UCookScopeFixtureAsset* Asset = NewObject<UCookScopeFixtureAsset>(
			Package,
			FName(AssetName),
			RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Asset);
		return {PackageName, Asset};
	}

	bool SaveFixture(const FFixture& Fixture)
	{
		Fixture.Asset->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Fixture.PackageName,
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_None;
		SaveArgs.Error = GError;
		return UPackage::SavePackage(Fixture.Asset->GetPackage(), Fixture.Asset, *Filename, SaveArgs);
	}

	FObjectFixture FindOrDuplicateFixture(
		const TCHAR* SourceObjectPath,
		const TCHAR* RelativePackage,
		const TCHAR* AssetName)
	{
		const FString PackageName = FString(ResourceFixtureRoot) / RelativePackage;
		const FString ObjectPath = PackageName + TEXT(".") + AssetName;
		if (UObject* Existing = LoadObject<UObject>(nullptr, *ObjectPath)) return {PackageName, Existing, false};
		UObject* Source = LoadObject<UObject>(nullptr, SourceObjectPath);
		if (!Source) return {PackageName, nullptr, false};
		UPackage* Package = CreatePackage(*PackageName);
		UObject* Asset = DuplicateObject(Source, Package, FName(AssetName));
		if (!Asset) return {PackageName, nullptr, false};
		Asset->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Asset);
		return {PackageName, Asset, true};
	}

	bool SaveObjectFixture(const FObjectFixture& Fixture)
	{
		if (!Fixture.Asset) return false;
		if (!Fixture.bCreated) return true;
		Fixture.Asset->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Fixture.PackageName,
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_None;
		SaveArgs.Error = GError;
		return UPackage::SavePackage(Fixture.Asset->GetPackage(), Fixture.Asset, *Filename, SaveArgs);
	}
}

UCookScopeFixtureBuilderCommandlet::UCookScopeFixtureBuilderCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
	ShowErrorCount = true;
	UseCommandletResultAsExitCode = true;
}

int32 UCookScopeFixtureBuilderCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	ParseCommandLine(*Params, Tokens, Switches);
	for (const FString& Switch : Switches)
	{
		if (!Switch.Equals(TEXT("run=CookScopeFixtureBuilder"), ESearchCase::IgnoreCase) &&
			!Switch.Equals(TEXT("unattended"), ESearchCase::IgnoreCase) &&
			!Switch.Equals(TEXT("nop4"), ESearchCase::IgnoreCase) &&
			!Switch.Equals(TEXT("nosplash"), ESearchCase::IgnoreCase) &&
			!Switch.Equals(TEXT("nullrhi"), ESearchCase::IgnoreCase) &&
			!Switch.Equals(TEXT("nosound"), ESearchCase::IgnoreCase) &&
			!Switch.Equals(TEXT("stdout"), ESearchCase::IgnoreCase) &&
			!Switch.Equals(TEXT("fullstdoutlogoutput"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogCookScopeFixtureBuilder, Error, TEXT("Unknown argument: -%s"), *Switch);
			return 3;
		}
	}
	if (!Tokens.IsEmpty())
	{
		UE_LOG(LogCookScopeFixtureBuilder, Error, TEXT("Unexpected positional argument: %s"), *Tokens[0]);
		return 3;
	}

	FFixture Target = FindOrCreateFixture(TEXT("Targets/DA_Target"), TEXT("DA_Target"));
	FFixture Hard = FindOrCreateFixture(TEXT("Sources/DA_Hard"), TEXT("DA_Hard"));
	FFixture Soft = FindOrCreateFixture(TEXT("Sources/DA_Soft"), TEXT("DA_Soft"));
	FFixture Searchable = FindOrCreateFixture(TEXT("Sources/DA_Searchable"), TEXT("DA_Searchable"));
	FFixture BadName = FindOrCreateFixture(TEXT("Naming/BadName"), TEXT("BadName"));
	FFixture Primary = FindOrCreateFixture(TEXT("Primary/DA_Primary"), TEXT("DA_Primary"));
	FFixture Candidate = FindOrCreateFixture(TEXT("Primary/DA_Candidate"), TEXT("DA_Candidate"));
	FFixture CycleA = FindOrCreateFixture(TEXT("Cycle/DA_CycleA"), TEXT("DA_CycleA"));
	FFixture CycleB = FindOrCreateFixture(TEXT("Cycle/DA_CycleB"), TEXT("DA_CycleB"));
	const TArray<FObjectFixture> ResourceFixtures = {
		FindOrDuplicateFixture(TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"), TEXT("Textures/T_Resource"), TEXT("T_Resource")),
		FindOrDuplicateFixture(TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("Meshes/SM_Resource"), TEXT("SM_Resource")),
		FindOrDuplicateFixture(TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube"), TEXT("Meshes/SK_Resource"), TEXT("SK_Resource")),
		FindOrDuplicateFixture(TEXT("/Engine/EngineSounds/1kSineTonePing.1kSineTonePing"), TEXT("Audio/S_Resource"), TEXT("S_Resource")),
	};

	Target.Asset->FixtureId = TEXT("fixture.target");
	Hard.Asset->FixtureId = TEXT("fixture.hard");
	Hard.Asset->HardReference = Target.Asset;
	Soft.Asset->FixtureId = TEXT("fixture.soft");
	Soft.Asset->SoftReference = TSoftObjectPtr<UCookScopeFixtureAsset>(Target.Asset);
	Searchable.Asset->FixtureId = TEXT("fixture.searchable");
	Searchable.Asset->SearchableTag = FGameplayTag::RequestGameplayTag(TEXT("CookScope.Search.Target"));
	BadName.Asset->FixtureId = TEXT("fixture.naming-invalid");
	Primary.Asset->FixtureId = TEXT("fixture.primary");
	Primary.Asset->BundledReference = TSoftObjectPtr<UCookScopeFixtureAsset>(Target.Asset);
	Candidate.Asset->FixtureId = TEXT("fixture.candidate-new");
	Candidate.Asset->BundledReference = TSoftObjectPtr<UCookScopeFixtureAsset>(Target.Asset);
	CycleA.Asset->FixtureId = TEXT("fixture.cycle-a");
	CycleA.Asset->HardReference = CycleB.Asset;
	CycleB.Asset->FixtureId = TEXT("fixture.cycle-b");
	CycleB.Asset->HardReference = CycleA.Asset;

	const TArray<FFixture> Fixtures = {Target, Hard, Soft, Searchable, BadName, Primary, Candidate, CycleA, CycleB};
	for (const FFixture& Fixture : Fixtures)
	{
		if (!Fixture.Asset || !SaveFixture(Fixture))
		{
			UE_LOG(LogCookScopeFixtureBuilder, Error, TEXT("Failed to save fixture: %s"), *Fixture.PackageName);
			return 4;
		}
	}
	for (const FObjectFixture& Fixture : ResourceFixtures)
	{
		if (!SaveObjectFixture(Fixture))
		{
			UE_LOG(LogCookScopeFixtureBuilder, Error, TEXT("Failed to save resource fixture: %s"), *Fixture.PackageName);
			return 4;
		}
	}

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.ScanPathsSynchronous({FixtureRoot, ResourceFixtureRoot}, true);
	UAssetManager& AssetManager = UAssetManager::Get();
	AssetManager.ScanPathForPrimaryAssets(
		FPrimaryAssetType(TEXT("CookScopeFixture")),
		FixtureRoot,
		UCookScopeFixtureAsset::StaticClass(),
		false,
		false,
		true);
	AssetManager.UpdateManagementDatabase(EUpdateManagementDatabaseFlags::BuildChunkMap | EUpdateManagementDatabaseFlags::ForceRefresh);

	UE_LOG(
		LogCookScopeFixtureBuilder,
		Display,
		TEXT("Generated %d deterministic fixtures under %s and %s"),
		Fixtures.Num() + ResourceFixtures.Num(),
		FixtureRoot,
		ResourceFixtureRoot);
	return 0;
}
