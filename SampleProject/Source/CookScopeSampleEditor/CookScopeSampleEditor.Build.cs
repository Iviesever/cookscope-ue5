using UnrealBuildTool;

public class CookScopeSampleEditor : ModuleRules
{
	public CookScopeSampleEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"CookScopeCore",
			"CookScopeEditor",
			"CookScopeSample",
			"DataValidation",
			"Engine",
			"GameplayTags",
			"UnrealEd"
		});
	}
}
