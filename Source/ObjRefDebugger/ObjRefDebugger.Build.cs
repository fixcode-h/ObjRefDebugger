// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ObjRefDebugger : ModuleRules
{
	public ObjRefDebugger(ReadOnlyTargetRules Target) : base(Target)
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
				"UnrealEd",
				"EditorStyle",
				"EditorWidgets",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"WorkspaceMenuStructure",
				"InputCore"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"EditorStyle",
				"EditorWidgets",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"WorkspaceMenuStructure",
				"PropertyEditor",
				"Kismet",
				"KismetCompiler",
				"AssetTools",
				"MainFrame",
				"EngineSettings",
				"DesktopPlatform",
				"ClassViewer"
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
