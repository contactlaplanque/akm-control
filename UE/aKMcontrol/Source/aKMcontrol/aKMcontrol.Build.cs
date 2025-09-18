// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class aKMcontrol : ModuleRules
{
	public aKMcontrol(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ImGui" });
		// Add OSC for client/server support
		PublicDependencyModuleNames.AddRange(new string[] { "OSC" });
		// JackAudioLink
		PrivateDependencyModuleNames.AddRange(new string[] { "UEJackAudioLink" });
		
	}
}
