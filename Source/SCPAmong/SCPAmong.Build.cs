// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class SCPAmong : ModuleRules
{
	public SCPAmong(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AIModule", "ProceduralMeshComponent", "SlateCore", "Slate", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] { "AdvancedSessions", "AdvancedSteamSessions" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		PrivateDependencyModuleNames.Add("OnlineSubsystem");

		PublicIncludePaths.Add("SCPAmong/Public");
		PrivateIncludePaths.Add("SCPAmong/Private");

		PrivateDependencyModuleNames.Add("OnlineSubsystem"); // TODO: compile out for android
		PrivateDependencyModuleNames.Add("OnlineSubsystemNull");
		DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
