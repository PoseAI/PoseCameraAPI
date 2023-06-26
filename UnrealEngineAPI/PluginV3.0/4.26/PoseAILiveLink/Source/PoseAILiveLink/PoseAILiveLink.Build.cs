// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PoseAILiveLink : ModuleRules
{
	public PoseAILiveLink(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Projects",
				"Networking",
				"Sockets",
				"LiveLink",
				"LiveLinkInterface",
				"Json",
				"JsonUtilities",
				"AnimationCore",
				"AnimGraphRuntime",
				// ... add other public dependencies that you statically link with here ...
			}
			);

		// some livelink functionality was moved to this module for UE5 so we need to include it in UE5 builds
		BuildVersion Version;
		if (BuildVersion.TryRead(BuildVersion.GetDefaultFileName(), out Version))
		{
			if (Version.MajorVersion == 5)
			{
				PublicDependencyModuleNames.AddRange(new string[] { "LiveLinkAnimationCore" });
			}
		}

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
			
				// ... add private dependencies that you statically link with here ...	
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
