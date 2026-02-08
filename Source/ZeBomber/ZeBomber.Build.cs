// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ZeBomber : ModuleRules
{
	public ZeBomber(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Landscape", "Niagara" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Slate UI (needed for FSlateApplication mouse button queries)
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
