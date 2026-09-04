#include "CookScopeFixtureAsset.h"

FPrimaryAssetId UCookScopeFixtureAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("CookScopeFixture")), GetFName());
}

