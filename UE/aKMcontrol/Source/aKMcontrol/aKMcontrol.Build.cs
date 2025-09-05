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
		// UI (ImGui is public as we expose its types in public headers)
		//PrivateDependencyModuleNames.AddRange(new string[] { "ImGui" });
		// JackAudioLink
		PrivateDependencyModuleNames.AddRange(new string[] { "UEJackAudioLink" });
	}
}
