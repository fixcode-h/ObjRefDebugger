// Copyright Epic Games, Inc. All Rights Reserved.

#include "SObjRefDebuggerClassPicker.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "ClassViewerModule.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "SObjRefDebuggerClassPicker"

void SObjRefDebuggerClassPicker::Construct(const FArguments& InArgs)
{
	// 保存参数
	CurrentSelectedClass = InArgs._InitialClass;
	OnClassSelected = InArgs._OnClassSelected;
	MaxHeight = InArgs._MaxHeight;
	MinWidth = InArgs._MinWidth;

	// 初始化类视图选项
	ClassViewerOptions.Mode = EClassViewerMode::ClassPicker;
	ClassViewerOptions.DisplayMode = EClassViewerDisplayMode::TreeView;
	ClassViewerOptions.bShowNoneOption = InArgs._ShowNoneOption;
	ClassViewerOptions.bExpandRootNodes = InArgs._ExpandRootNodes;
	ClassViewerOptions.NameTypeToDisplay = EClassViewerNameTypeToDisplay::Dynamic;

	// 创建组件
	ChildSlot
	[
		SAssignNew(ComboButton, SComboButton)
		.ContentPadding(2)
		.ButtonStyle(FEditorStyle::Get(), "ToolBar.Button")
		.ForegroundColor(FSlateColor::UseForeground())
		.OnGetMenuContent(this, &SObjRefDebuggerClassPicker::GetMenuContent)
		.ToolTipText(LOCTEXT("ClassPickerTooltip", "选择要搜索的类"))
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("ClassIcon.Class"))
			]
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(this, &SObjRefDebuggerClassPicker::GetButtonText)
			]
		]
	];

	// 设置初始选择
	if (CurrentSelectedClass)
	{
		SetSelectedClass(CurrentSelectedClass);
	}
}

TSharedRef<SWidget> SObjRefDebuggerClassPicker::GetMenuContent()
{
	// 加载类查看器模块
	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
	
	// 创建类选择器控件
	TSharedRef<SWidget> ClassPickerWidget = ClassViewerModule.CreateClassViewer(
		ClassViewerOptions, 
		FOnClassPicked::CreateSP(this, &SObjRefDebuggerClassPicker::OnClassPickedInternal)
	);
	
	// 将类选择器包装在滚动框中
	return SNew(SBox)
		.MaxDesiredHeight(MaxHeight)
		.MinDesiredWidth(MinWidth)
		[
			SNew(SScrollBox)
			+SScrollBox::Slot()
			[
				ClassPickerWidget
			]
		];
}

void SObjRefDebuggerClassPicker::OnClassPickedInternal(UClass* InClass)
{
	CurrentSelectedClass = InClass;
	
	// 转发给用户提供的委托
	if (OnClassSelected.IsBound())
	{
		OnClassSelected.Execute(InClass);
	}
	
	// 关闭下拉菜单
	FSlateApplication::Get().DismissAllMenus();
}

FText SObjRefDebuggerClassPicker::GetButtonText() const
{
	return LOCTEXT("SelectClass", "选择类...");
}

void SObjRefDebuggerClassPicker::SetSelectedClass(UClass* InClass)
{
	CurrentSelectedClass = InClass;
}

#undef LOCTEXT_NAMESPACE 