#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CookScopeFixtureAsset.generated.h"

UCLASS(BlueprintType)
class COOKSCOPESAMPLE_API UCookScopeFixtureAsset final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, Category = "CookScope Fixture")
	TObjectPtr<UCookScopeFixtureAsset> HardReference;

	UPROPERTY(EditAnywhere, Category = "CookScope Fixture")
	TSoftObjectPtr<UCookScopeFixtureAsset> SoftReference;

	UPROPERTY(EditAnywhere, Category = "CookScope Fixture", meta = (AssetBundles = "Default"))
	TSoftObjectPtr<UCookScopeFixtureAsset> BundledReference;

	UPROPERTY(EditAnywhere, Category = "CookScope Fixture")
	FGameplayTag SearchableTag;

	UPROPERTY(EditAnywhere, Category = "CookScope Fixture", AssetRegistrySearchable)
	FName FixtureId;
};
