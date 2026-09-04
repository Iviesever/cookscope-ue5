using UnrealBuildTool;

public class CookScopeCommandlet : ModuleRules
{
	public CookScopeCommandlet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"CookScopeCore",
			"CookScopeEditor",
			"Engine",
			"Projects"
		});
	}
}
