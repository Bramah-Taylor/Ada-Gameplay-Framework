// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

using UnrealBuildTool;

public class AdaGameplayEditor : ModuleRules
{
	public AdaGameplayEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(new string[] 
		{

		});	
		
		PrivateIncludePaths.AddRange(new string[] 
		{

		});
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AdaCore", 
			"AdaGameplay",
			"GameplayTags",
			"DataRegistry",
			"AssetTools"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ClassViewer",
			"EditorFramework",
			"UnrealEd",
			"Slate",
			"SlateCore",
		});
		
		DynamicallyLoadedModuleNames.AddRange(new string[]
		{
				
		});
	}
}
