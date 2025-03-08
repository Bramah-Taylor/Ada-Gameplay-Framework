// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

using UnrealBuildTool;

public class AdaCore : ModuleRules
{
	public AdaCore(ReadOnlyTargetRules Target) : base(Target)
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
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
		});
		
		DynamicallyLoadedModuleNames.AddRange(new string[]
		{
				
		});
	}
}
