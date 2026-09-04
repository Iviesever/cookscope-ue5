#include "CookScopeFixtureBuilderCommandlet.h"

#include "CookScopeFixtureAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogCookScopeFixtureBuilder, Log, All);

namespace
{
	constexpr TCHAR FixtureRoot[] = TEXT("/Game/CookScopeFixtures");
	constexpr TCHAR P0FixtureRoot[] = TEXT("/Game/CookScopeP0Fixtures");
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

	FFixture FindOrCreateFixtureAtRoot(const TCHAR* Root, const TCHAR* RelativePackage, const TCHAR* AssetName)
	{
		const FString PackageName = FString(Root) / RelativePackage;
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

	FFixture FindOrCreateFixture(const TCHAR* RelativePackage, const TCHAR* AssetName)
	{
		return FindOrCreateFixtureAtRoot(FixtureRoot, RelativePackage, AssetName);
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

	FObjectFixture FindOrCreateRedirector(UObject* Destination)
	{
		const FString PackageName = FString(P0FixtureRoot) / TEXT("Redirectors/OldTarget");
		UPackage* Package = LoadPackage(nullptr, *PackageName, LOAD_None);
		if (Package)
		{
			if (UObjectRedirector* Existing = FindObject<UObjectRedirector>(Package, TEXT("OldTarget")))
				return {PackageName, Existing, false};
		}
		else
		{
			Package = CreatePackage(*PackageName);
		}
		UObjectRedirector* Redirector = NewObject<UObjectRedirector>(
			Package,
			TEXT("OldTarget"),
			RF_Public | RF_Standalone | RF_Transactional);
		if (!Redirector) return {PackageName, nullptr, false};
		Redirector->DestinationObject = Destination;
		FAssetRegistryModule::AssetCreated(Redirector);
		return {PackageName, Redirector, true};
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
	FFixture EditorOnly = FindOrCreateFixtureAtRoot(P0FixtureRoot, TEXT("EditorOnly/DA_EditorOnly"), TEXT("DA_EditorOnly"));
	FFixture Runtime = FindOrCreateFixtureAtRoot(P0FixtureRoot, TEXT("Runtime/DA_Runtime"), TEXT("DA_Runtime"));
	FFixture MissingRef = FindOrCreateFixtureAtRoot(P0FixtureRoot, TEXT("MissingRef/DA_MissingRef"), TEXT("DA_MissingRef"));
	FFixture Conflict = FindOrCreateFixtureAtRoot(P0FixtureRoot, TEXT("Primary/DA_Conflict"), TEXT("DA_Conflict"));
	FFixture DuplicateOne = FindOrCreateFixtureAtRoot(P0FixtureRoot, TEXT("Duplicate/One/DA_Duplicate"), TEXT("DA_Duplicate"));
	FFixture DuplicateTwo = FindOrCreateFixtureAtRoot(P0FixtureRoot, TEXT("Duplicate/Two/DA_Duplicate"), TEXT("DA_Duplicate"));
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
	Primary.Asset->HardReference = Runtime.Asset;
	Candidate.Asset->FixtureId = TEXT("fixture.candidate-new");
	Candidate.Asset->BundledReference = TSoftObjectPtr<UCookScopeFixtureAsset>(Target.Asset);
	CycleA.Asset->FixtureId = TEXT("fixture.cycle-a");
	CycleA.Asset->HardReference = CycleB.Asset;
	CycleB.Asset->FixtureId = TEXT("fixture.cycle-b");
	CycleB.Asset->HardReference = CycleA.Asset;
	EditorOnly.Asset->FixtureId = TEXT("fixture.editor-only-cooked");
	Runtime.Asset->FixtureId = TEXT("fixture.runtime-to-editor");
	Runtime.Asset->HardReference = EditorOnly.Asset;
	MissingRef.Asset->FixtureId = TEXT("fixture.missing-reference");
	MissingRef.Asset->MissingReference = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/CookScopeFixtures/DoesNotExist.Missing")));
	Conflict.Asset->FixtureId = TEXT("fixture.asset-manager-conflict");
	Conflict.Asset->BundledReference = TSoftObjectPtr<UCookScopeFixtureAsset>(Target.Asset);
	Conflict.Asset->AlwaysCook = TEXT("true");
	Conflict.Asset->NeverCook = TEXT("true");
	DuplicateOne.Asset->FixtureId = TEXT("fixture.ambiguous-one");
	DuplicateTwo.Asset->FixtureId = TEXT("fixture.ambiguous-two");

	const TArray<FFixture> Fixtures = {
		Target, Hard, Soft, Searchable, BadName, Primary, Candidate, CycleA, CycleB,
		EditorOnly, Runtime, MissingRef, Conflict, DuplicateOne, DuplicateTwo};
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
	const FObjectFixture Redirector = FindOrCreateRedirector(Target.Asset);
	if (!SaveObjectFixture(Redirector))
	{
		UE_LOG(LogCookScopeFixtureBuilder, Error, TEXT("Failed to save redirector fixture: %s"), *Redirector.PackageName);
		return 4;
	}

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.ScanPathsSynchronous({FixtureRoot, P0FixtureRoot, ResourceFixtureRoot}, true);
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
		TEXT("Generated %d deterministic fixtures under %s, %s, and %s"),
		Fixtures.Num() + ResourceFixtures.Num() + 1,
		FixtureRoot,
		P0FixtureRoot,
		ResourceFixtureRoot);
	return 0;
}
