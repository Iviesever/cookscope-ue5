#pragma once

#include "Commandlets/Commandlet.h"
#include "CookScopeFixtureBuilderCommandlet.generated.h"

UCLASS()
class COOKSCOPESAMPLEEDITOR_API UCookScopeFixtureBuilderCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UCookScopeFixtureBuilderCommandlet();
	virtual int32 Main(const FString& Params) override;
};

