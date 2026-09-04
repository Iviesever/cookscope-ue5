#pragma once

#include "EditorValidatorBase.h"
#include "CookScopeValidator.generated.h"

UCLASS()
class COOKSCOPEEDITOR_API UCookScopeValidator final : public UEditorValidatorBase
{
	GENERATED_BODY()

protected:
	virtual bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InObject,
		FDataValidationContext& InContext) const override;

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& Context) override;
};

