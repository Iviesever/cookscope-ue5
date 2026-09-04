using UnrealBuildTool;

public class CookScopeEditor : ModuleRules
{
	public CookScopeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CookScopeCore",
			"DataValidation"
		});
		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"LevelEditor",
			"Projects",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
