// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

// 前向声明
class SObjRefDebuggerWindow;

class FObjRefDebuggerModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** 创建调试器窗口 */
	TSharedRef<SDockTab> OnSpawnObjRefDebuggerTab(const FSpawnTabArgs& SpawnTabArgs);
	
	/** 注册菜单项 */
	void RegisterMenuExtensions();
	
	/** 移除菜单项 */
	void UnregisterMenuExtensions();

	static TSharedPtr<FTabManager::FLayout> WindowLayout;
};
