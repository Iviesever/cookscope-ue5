#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class SDockTab;
class FSpawnTabArgs;
class FCookScopeEditorSession;

class COOKSCOPEEDITOR_API FCookScopeEditorModule final : public IModuleInterface
{
public:
	static FName GetTabName();
	static TSharedPtr<SDockTab> InvokeTab();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<SDockTab> SpawnCookScopeTab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedPtr<FCookScopeEditorSession> Session;
};
