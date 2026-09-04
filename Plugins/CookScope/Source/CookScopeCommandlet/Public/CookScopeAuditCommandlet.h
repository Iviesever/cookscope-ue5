#pragma once

#include "Commandlets/Commandlet.h"
#include "CookScopeAuditCommandlet.generated.h"

UCLASS()
class COOKSCOPECOMMANDLET_API UCookScopeAuditCommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UCookScopeAuditCommandlet();
	virtual int32 Main(const FString& Params) override;
};

