// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboButton.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"

DECLARE_DELEGATE_OneParam(FOnClassSelected, UClass*);

/** 对象引用调试器类选择器组件 */
class SObjRefDebuggerClassPicker : public SCompoundWidget
{
public:
	/** 定义按钮文本获取的委托类型 */
	DECLARE_DELEGATE_RetVal(FText, FOnGetButtonText);

	SLATE_BEGIN_ARGS(SObjRefDebuggerClassPicker)
		: _InitialClass(nullptr)
		, _MaxHeight(400.0f)
		, _MinWidth(300.0f)
		, _ShowNoneOption(false)
		, _ExpandRootNodes(true)
	{}
		/** 初始选择的类 */
		SLATE_ARGUMENT(UClass*, InitialClass)

		/** 下拉框的最大高度 */
		SLATE_ARGUMENT(float, MaxHeight)

		/** 下拉框的最小宽度 */
		SLATE_ARGUMENT(float, MinWidth)

		/** 是否显示None选项 */
		SLATE_ARGUMENT(bool, ShowNoneOption)

		/** 是否展开根节点 */
		SLATE_ARGUMENT(bool, ExpandRootNodes)

		/** 类选择回调 */
		SLATE_EVENT(FOnClassSelected, OnClassSelected)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 设置选中的类 */
	void SetSelectedClass(UClass* InClass);
	
	/** 获取选中的类 */
	UClass* GetSelectedClass() const { return CurrentSelectedClass; }

private:
	/** 获取下拉菜单内容 */
	TSharedRef<SWidget> GetMenuContent();

	/** 类选择回调包装，会自动关闭菜单 */
	void OnClassPickedInternal(UClass* InClass);

	/** 获取按钮文本 */
	FText GetButtonText() const;

private:
	/** 类视图初始化选项 */
	FClassViewerInitializationOptions ClassViewerOptions;
	
	/** 当前选中的类 */
	UClass* CurrentSelectedClass;
	
	/** 类选择回调 */
	FOnClassSelected OnClassSelected;
	
	/** 下拉框最大高度 */
	float MaxHeight;
	
	/** 下拉框最小宽度 */
	float MinWidth;
	
	/** 组合按钮控件 */
	TSharedPtr<SComboButton> ComboButton;
}; 