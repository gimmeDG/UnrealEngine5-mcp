using UnrealBuildTool;

public class UnrealEngineMCP : ModuleRules
{
	public UnrealEngineMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Networking",
				"Sockets",
				"HTTP",
				"Json",
				"JsonUtilities",
				"DeveloperSettings",
				// GAS (Gameplay Ability System) support
				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks"
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"EditorScriptingUtilities",
				"EditorSubsystem",
				"Slate",
				"SlateCore",
				"UMG",
				"Kismet",
				"KismetCompiler",
				"BlueprintGraph",
				"Projects",
				"AssetRegistry",
				"PropertyEditor",
				"ToolMenus",
				"BlueprintEditorLibrary",
				"UMGEditor",
				"WorldPartitionEditor",
				// PCG (Procedural Content Generation) support
				"PCG"
			}
		);
		
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("PythonScriptPlugin");
		}
	}
}