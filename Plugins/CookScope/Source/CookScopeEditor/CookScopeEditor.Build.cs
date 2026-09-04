using UnrealBuildTool;

public class CookScopeEditor : ModuleRules
{
	public CookScopeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CookScopeCore"
		});
		PrivateDependencyModuleNames.AddRange(new string[] {
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"Engine",
			"LevelEditor",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd"
		});
	}
}

