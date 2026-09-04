#include "CookScopeValidator.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/AutomationTest.h"
#include "Misc/DataValidation.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCookScopeDataValidationTest,
	"CookScope.PACT30.DataValidationReuse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	FAssetData FindAssetData(const TCHAR* PackageName)
	{
		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		TArray<FAssetData> Assets;
		Registry.GetAssetsByPackageName(FName(PackageName), Assets, true);
		return Assets.IsEmpty() ? FAssetData() : Assets[0];
	}
}

bool FCookScopeDataValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FAssetData BadData = FindAssetData(TEXT("/Game/CookScopeFixtures/Naming/BadName"));
	const FAssetData GoodData = FindAssetData(TEXT("/Game/CookScopeFixtures/Targets/DA_Target"));
	TestTrue(TEXT("Bad naming fixture exists in Asset Registry"), BadData.IsValid());
	TestTrue(TEXT("Good target fixture exists in Asset Registry"), GoodData.IsValid());
	if (!BadData.IsValid() || !GoodData.IsValid()) return false;

	UCookScopeValidator* BadValidator = NewObject<UCookScopeValidator>();
	FDataValidationContext BadContext;
	const EDataValidationResult BadResult = BadValidator->ValidateLoadedAsset(BadData, BadData.GetAsset(), BadContext);
	TestEqual(TEXT("Shared naming rule fails bad fixture"), BadResult, EDataValidationResult::Invalid);
	TestTrue(TEXT("Bad fixture produces a Data Validation error"), BadContext.GetNumErrors() > 0);

	UCookScopeValidator* GoodValidator = NewObject<UCookScopeValidator>();
	FDataValidationContext GoodContext;
	const EDataValidationResult GoodResult = GoodValidator->ValidateLoadedAsset(GoodData, GoodData.GetAsset(), GoodContext);
	TestEqual(TEXT("Shared naming rule passes good fixture"), GoodResult, EDataValidationResult::Valid);
	TestEqual(TEXT("Good fixture has no Data Validation errors"), GoodContext.GetNumErrors(), 0u);
	return true;
}

#endif

