#include "CookScopeAssetScanner.h"

#include "cookscope/graph.h"

#include "Misc/AutomationTest.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCookScopeAssetRegistryScanTest,
	"CookScope.PACT20.RealAssetRegistryScan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	const cookscope::AssetRecord* FindAsset(const cookscope::Snapshot& Snapshot, std::string_view ObjectPath)
	{
		const auto Found = std::find_if(Snapshot.assets.begin(), Snapshot.assets.end(), [&](const cookscope::AssetRecord& Asset) {
			return Asset.objectPath == ObjectPath;
		});
		return Found == Snapshot.assets.end() ? nullptr : &*Found;
	}

	bool HasEdge(const cookscope::AssetRecord& Asset, std::string_view Target, cookscope::DependencyKind Kind)
	{
		return std::any_of(Asset.dependencies.begin(), Asset.dependencies.end(), [&](const cookscope::DependencyEdge& Edge) {
			return Edge.target == Target && Edge.kind == Kind;
		});
	}

	bool HasKind(const cookscope::AssetRecord& Asset, cookscope::DependencyKind Kind)
	{
		return std::any_of(Asset.dependencies.begin(), Asset.dependencies.end(), [&](const cookscope::DependencyEdge& Edge) {
			return Edge.kind == Kind;
		});
	}
}

bool FCookScopeAssetRegistryScanTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FCookScopeScanResult Result = FCookScopeAssetScanner::ScanPath(
		TEXT("/Game/CookScopeFixtures"),
		TEXT("0000000000000000000000000000000000000000"));
	TestTrue(TEXT("Real Asset Registry scan succeeds"), Result.bSuccess);
	if (!Result.bSuccess)
	{
		AddError(Result.Error);
		return false;
	}
	TestEqual(TEXT("All deterministic fixtures are scanned"), static_cast<int32>(Result.Snapshot.assets.size()), 9);

	const cookscope::AssetRecord* Target = FindAsset(Result.Snapshot, "/Game/CookScopeFixtures/Targets/DA_Target.DA_Target");
	const cookscope::AssetRecord* Hard = FindAsset(Result.Snapshot, "/Game/CookScopeFixtures/Sources/DA_Hard.DA_Hard");
	const cookscope::AssetRecord* Soft = FindAsset(Result.Snapshot, "/Game/CookScopeFixtures/Sources/DA_Soft.DA_Soft");
	const cookscope::AssetRecord* Searchable = FindAsset(Result.Snapshot, "/Game/CookScopeFixtures/Sources/DA_Searchable.DA_Searchable");
	const cookscope::AssetRecord* Primary = FindAsset(Result.Snapshot, "/Game/CookScopeFixtures/Primary/DA_Primary.DA_Primary");
	TestNotNull(TEXT("Target fixture is present"), Target);
	TestNotNull(TEXT("Hard fixture is present"), Hard);
	TestNotNull(TEXT("Soft fixture is present"), Soft);
	TestNotNull(TEXT("Searchable fixture is present"), Searchable);
	TestNotNull(TEXT("Primary fixture is present"), Primary);
	if (!Target || !Hard || !Soft || !Searchable || !Primary)
	{
		return false;
	}

	TestTrue(TEXT("Package disk size is reported as measured data"),
		Target->diskSize.kind == cookscope::MeasurementKind::PackageDisk && Target->diskSize.bytes.value_or(0) > 0);
	TestTrue(TEXT("Cooked size remains unavailable before a real Cook"),
		Target->cookedSize.kind == cookscope::MeasurementKind::Unavailable && !Target->cookedSize.bytes.has_value());
	TestTrue(TEXT("Hard dependency remains typed"), HasEdge(*Hard, Target->objectPath, cookscope::DependencyKind::Hard));
	TestTrue(TEXT("Soft dependency remains typed"), HasEdge(*Soft, Target->objectPath, cookscope::DependencyKind::Soft));
	TestTrue(TEXT("Searchable Name dependency remains typed"), HasKind(*Searchable, cookscope::DependencyKind::SearchableName));
	TestTrue(TEXT("Primary Asset ID is captured"), Primary->primaryAssetId == std::optional<std::string>("CookScopeFixture:DA_Primary"));
	TestTrue(TEXT("Default bundle is captured"),
		std::find(Primary->assetBundles.begin(), Primary->assetBundles.end(), "Default") != Primary->assetBundles.end());
	TestTrue(TEXT("Configured Chunk ID is captured"),
		std::find(Primary->chunkIds.begin(), Primary->chunkIds.end(), 1) != Primary->chunkIds.end());
	TestTrue(TEXT("Manage dependency remains typed"), HasEdge(*Primary, Target->objectPath, cookscope::DependencyKind::Manage));

	const cookscope::GraphBuildResult Graph = cookscope::BuildDependencyGraph(Result.Snapshot, cookscope::OperationLimits{});
	TestTrue(TEXT("Scanned snapshot builds a complete graph"), Graph.state == cookscope::OperationState::Complete);
	const cookscope::CyclesResult Cycles = cookscope::FindCycles(Graph.graph, cookscope::DependencyMask::All(), cookscope::OperationLimits{});
	TestTrue(TEXT("Real registry hard-reference cycle is detected"), !Cycles.cycles.empty());
	const std::array<std::string_view, 1> Roots{Primary->objectPath};
	const cookscope::WhyCookedResult Why = cookscope::ExplainWhyCooked(
		Graph.graph, Roots, Target->objectPath, cookscope::DependencyMask::All(), cookscope::OperationLimits{});
	TestTrue(TEXT("Primary fixture explains why target is cooked"), Why.found && !Why.steps.empty());
	return true;
}

#endif
