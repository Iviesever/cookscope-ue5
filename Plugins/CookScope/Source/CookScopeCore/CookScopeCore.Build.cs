using UnrealBuildTool;

public class CookScopeCore : ModuleRules
{
	public CookScopeCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bEnableExceptions = false;
		bUseRTTI = false;
		PublicDependencyModuleNames.AddRange(new string[] { "Core" });
	}
}

