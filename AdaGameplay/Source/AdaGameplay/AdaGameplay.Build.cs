// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

using UnrealBuildTool;

public class AdaGameplay : ModuleRules
{
	public AdaGameplay(ReadOnlyTargetRules Target) : base(Target)
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
			"AdaCore", 
			"GameplayTags",
			"DataRegistry"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"DeveloperSettings"
		});
		
		DynamicallyLoadedModuleNames.AddRange(new string[]
		{
				
		});
	}
}
