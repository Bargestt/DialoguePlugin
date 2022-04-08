

using UnrealBuildTool;

public class DialoguePluginEditor : ModuleRules
{
	public DialoguePluginEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"DialoguePlugin"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",

				"InputCore",
				"DeveloperSettings",

				"UnrealEd",
				"KismetWidgets",
				"PropertyEditor",

				"EditorStyle",
				"Projects",

                "GraphEditor",
				"ApplicationCore",
				"ToolMenus",
			}
			);	
		
	}
}
