#include "CookScopeAssetScanner.h"
#include "CookScopeCookSnapshotReader.h"

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#include <algorithm>
#include <string_view>

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCookScopeCookedRegistryTest,
	"CookScope.PACT40.CookedRegistrySnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	const cookscope::AssetRecord* FindAsset(const cookscope::Snapshot& Snapshot, std::string_view Path)
	{
		const auto Found = std::find_if(Snapshot.assets.begin(), Snapshot.assets.end(), [&](const cookscope::AssetRecord& Asset) {
			return Asset.objectPath == Path;
		});
		return Found == Snapshot.assets.end() ? nullptr : &*Found;
	}
}

bool FCookScopeCookedRegistryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FCookScopeScanResult Source = FCookScopeAssetScanner::ScanPath(
		TEXT("/Game/CookScopeFixtures"),
		TEXT("0000000000000000000000000000000000000000"));
	TestTrue(TEXT("Source snapshot is available"), Source.bSuccess);
	if (!Source.bSuccess) return false;

	const FString RegistryPath = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Cooked/Windows/CookScopeSample/Metadata/DevelopmentAssetRegistry.bin"));
	const FCookScopeCookMergeResult Merge = FCookScopeCookSnapshotReader::MergeDevelopmentRegistry(
		RegistryPath,
		TEXT("Windows"),
		TEXT("Development"),
		Source.Snapshot);
	TestTrue(TEXT("Development Asset Registry merges into snapshot"), Merge.bSuccess);
	if (!Merge.bSuccess)
	{
		AddError(Merge.Error);
		return false;
	}

	const cookscope::AssetRecord* Primary = FindAsset(
		Merge.Snapshot,
		"/Game/CookScopeFixtures/Primary/DA_Primary.DA_Primary");
	const cookscope::AssetRecord* BadName = FindAsset(
		Merge.Snapshot,
		"/Game/CookScopeFixtures/Naming/BadName.BadName");
	TestNotNull(TEXT("Primary fixture remains in merged snapshot"), Primary);
	TestNotNull(TEXT("Uncooked naming fixture remains in merged snapshot"), BadName);
	if (!Primary || !BadName) return false;
	TestTrue(TEXT("Primary fixture gets actual Cook bytes"),
		Primary->cookedSize.kind == cookscope::MeasurementKind::ActualCooked && Primary->cookedSize.bytes.value_or(0) > 0);
	TestTrue(TEXT("Uncooked fixture does not get fabricated Cook bytes"),
		BadName->cookedSize.kind == cookscope::MeasurementKind::Unavailable && !BadName->cookedSize.bytes.has_value());
	TestEqual(TEXT("Cook platform provenance is updated"), FString(UTF8_TO_TCHAR(Merge.Snapshot.provenance.platform.c_str())), FString(TEXT("Windows")));
	TestEqual(TEXT("Cook configuration provenance is updated"), FString(UTF8_TO_TCHAR(Merge.Snapshot.provenance.cookConfiguration.c_str())), FString(TEXT("Development")));
	return true;
}

#endif

