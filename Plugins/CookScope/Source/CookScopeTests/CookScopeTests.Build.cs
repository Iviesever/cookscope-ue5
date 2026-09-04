using UnrealBuildTool;

public class CookScopeTests : ModuleRules
{
	public CookScopeTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"CookScopeCommandlet",
			"CookScopeEditor",
			"Engine",
			"Slate",
			"SlateCore",
			"UnrealEd"
		});
	}
}

