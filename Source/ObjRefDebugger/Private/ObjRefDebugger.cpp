// Copyright Epic Games, Inc. All Rights Reserved.

#include "ObjRefDebugger.h"
#include "SObjRefDebuggerWindow.h"
#include "Widgets/Layout/SBox.h"
#include "EditorStyleSet.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FObjRefDebuggerModule"

static const FName ObjRefDebuggerTabName("ObjRefDebuggerTab");
TSharedPtr<FTabManager::FLayout> FObjRefDebuggerModule::WindowLayout;

void FObjRefDebuggerModule::StartupModule()
{
	// 注册调试器标签页
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ObjRefDebuggerTabName, FOnSpawnTab::CreateRaw(this, &FObjRefDebuggerModule::OnSpawnObjRefDebuggerTab))
		.SetDisplayName(LOCTEXT("FObjRefDebuggerTabTitle", "对象引用调试器"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	RegisterMenuExtensions();
}

void FObjRefDebuggerModule::ShutdownModule()
{
	UnregisterMenuExtensions();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ObjRefDebuggerTabName);
}

TSharedRef<SDockTab> FObjRefDebuggerModule::OnSpawnObjRefDebuggerTab(const FSpawnTabArgs& SpawnTabArgs)
{
	const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SObjRefDebuggerWindow)
		];

	return DockTab;
}

void FObjRefDebuggerModule::RegisterMenuExtensions()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	UToolMenu* MainMenu = ToolMenus->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& Section = MainMenu->FindOrAddSection("WindowLayout");
	
	Section.AddMenuEntry(
		"ObjRefDebugger",
		LOCTEXT("ObjRefDebuggerMenuLabel", "对象引用调试器"),
		LOCTEXT("ObjRefDebuggerMenuTooltip", "打开对象引用调试器窗口"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			FGlobalTabmanager::Get()->TryInvokeTab(ObjRefDebuggerTabName);
		}))
	);
}

void FObjRefDebuggerModule::UnregisterMenuExtensions()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (ToolMenus)
	{
		ToolMenus->RemoveSection("LevelEditor.MainMenu.Window", "WindowLayout");
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FObjRefDebuggerModule, ObjRefDebugger)