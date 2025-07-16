// Copyright Epic Games, Inc. All Rights Reserved.

#include "SObjRefDebuggerWindow.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Images/SImage.h"
#include "EditorStyleSet.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "SObjRefDebuggerWindow"

void SObjRefDebuggerWindow::Construct(const FArguments& InArgs)
{
	bIsSearching = false;
	LastSearchTime = 0.0f;
	CurrentFilterOptions = FSearchFilterOptions();
	CurrentStatistics = FSearchStatistics();
	CurrentViewMode = 0;
	bShowAdvancedOptions = false;
	LastRefreshTime = FDateTime::Now();

	// 初始化视图模式选项
	ViewModeOptions.Add(MakeShareable(new FText(LOCTEXT("ListViewMode", "列表视图"))));
	ViewModeOptions.Add(MakeShareable(new FText(LOCTEXT("DetailViewMode", "详细视图"))));
	ViewModeOptions.Add(MakeShareable(new FText(LOCTEXT("GraphViewMode", "图表视图"))));

	ChildSlot
	[
		SNew(SVerticalBox)

		// 顶部工具栏
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f, 5.0f, 5.0f, 0.0f)
		[
			BuildTopToolbar()
		]

		// 搜索区域
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			BuildSearchArea()
		]

		// 过滤选项（可折叠）
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f, 0.0f, 5.0f, 5.0f)
		[
			BuildFilterOptions()
		]

		// 统计信息面板
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f, 0.0f, 5.0f, 5.0f)
		[
			BuildStatisticsPanel()
		]

		// 主内容区域
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(5.0f)
		[
			BuildMainContent()
		]
	];
}

TSharedRef<SWidget> SObjRefDebuggerWindow::BuildTopToolbar()
{
	return SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(10.0f)
		[
			SNew(SHorizontalBox)

			// 标题和图标
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 20, 0)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 10, 0)
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("Icons.Search"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ToolTitle", "对象引用调试器"))
					.Font(FEditorStyle::GetFontStyle("BoldFont"))
				]
			]

			// 视图模式切换
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 20, 0)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 5, 0)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ViewMode", "视图模式:"))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SComboBox<TSharedPtr<FText>>)
					.OptionsSource(&ViewModeOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FText> Item)
					{
						return SNew(STextBlock).Text(*Item);
					})
					.OnSelectionChanged(this, &SObjRefDebuggerWindow::OnViewModeSelectionChanged)
					.Content()
					[
						SNew(STextBlock)
						.Text(this, &SObjRefDebuggerWindow::GetCurrentViewModeText)
					]
				]
			]

			// 快速操作按钮
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ForceGC", "强制GC"))
					.OnClicked(this, &SObjRefDebuggerWindow::OnForceGCClicked)
					.ToolTipText(LOCTEXT("ForceGCTooltip", "强制执行垃圾回收并自动刷新搜索结果"))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("ClearResults", "清除结果"))
					.OnClicked(this, &SObjRefDebuggerWindow::OnClearResultsClicked)
					.ToolTipText(LOCTEXT("ClearResultsTooltip", "清除当前搜索结果"))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Refresh", "刷新"))
					.OnClicked(this, &SObjRefDebuggerWindow::OnRefreshClicked)
					.ToolTipText(LOCTEXT("RefreshTooltip", "刷新当前搜索结果"))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("Export", "导出"))
					.OnClicked(this, &SObjRefDebuggerWindow::OnExportResultsClicked)
					.ToolTipText(LOCTEXT("ExportTooltip", "导出搜索结果到文件"))
				]
			]
		];
}

TSharedRef<SWidget> SObjRefDebuggerWindow::BuildSearchArea()
{
	return SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(10.0f)
		[
			SNew(SVerticalBox)

			// 主要搜索区域
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				// 类名输入框
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ClassNameLabel", "选择UClass:"))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 2, 0, 0)
					[
						SAssignNew(ClassPicker, SObjRefDebuggerClassPicker)
						.OnClassSelected(this, &SObjRefDebuggerWindow::OnClassSelected)
					]
				]

				// 搜索按钮
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Bottom)
				.Padding(10.0f, 0, 0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("SearchButton", "搜索实例"))
					.OnClicked(this, &SObjRefDebuggerWindow::OnSearchClicked)
					.IsEnabled_Lambda([this]() { return !bIsSearching; })
				]
			]

			// 已选择的类列表
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 10, 0, 0)
			[
				SNew(SVerticalBox)
				
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::FromString(FString::Printf(TEXT("已选择的类 (%d):"), CurrentClassNames.Num()));
					})
					.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
					.Visibility_Lambda([this]()
					{
						return CurrentClassNames.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
					})
				]
				
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 5, 0, 0)
				[
					SAssignNew(SelectedClassListView, SListView<TSharedPtr<FString>>)
					.ListItemsSource(&CurrentClassNames)
					.OnGenerateRow(this, &SObjRefDebuggerWindow::OnGenerateSelectedClassRow)
					.SelectionMode(ESelectionMode::None)
					.Visibility_Lambda([this]()
					{
						return CurrentClassNames.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
					})
				]
			]

			// 搜索进度条
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 10, 0, 0)
			[
				SAssignNew(SearchProgressBar, SProgressBar)
				.Visibility_Lambda([this]()
				{
					return bIsSearching ? EVisibility::Visible : EVisibility::Collapsed;
				})
			]
		];
}

TSharedRef<SWidget> SObjRefDebuggerWindow::BuildFilterOptions()
{
	return SNew(SExpandableArea)
		.InitiallyCollapsed(true)
		.HeaderContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("FilterOptionsTitle", "过滤选项"))
			.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
		]
		.BodyContent()
		[
			SNew(SGridPanel)
			.FillColumn(0, 1.0f)
			.FillColumn(1, 1.0f)
			.FillColumn(2, 1.0f)

			// 第一行：世界过滤
			+ SGridPanel::Slot(0, 0)
			.Padding(5)
			[
				SAssignNew(IncludeEditorWorldCheckBox, SCheckBox)
				.IsChecked(ECheckBoxState::Checked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					CurrentFilterOptions.bIncludeEditorWorld = (NewState == ECheckBoxState::Checked);
					OnFilterOptionChanged();
				})
				[
					SNew(STextBlock)
					.Text(LOCTEXT("IncludeEditorWorld", "包含编辑器世界"))
				]
			]

			+ SGridPanel::Slot(1, 0)
			.Padding(5)
			[
				SAssignNew(IncludePIEWorldCheckBox, SCheckBox)
				.IsChecked(ECheckBoxState::Checked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					CurrentFilterOptions.bIncludePIEWorld = (NewState == ECheckBoxState::Checked);
					OnFilterOptionChanged();
				})
				[
					SNew(STextBlock)
					.Text(LOCTEXT("IncludePIEWorld", "包含PIE世界"))
				]
			]

			+ SGridPanel::Slot(2, 0)
			.Padding(5)
			[
				SNew(SCheckBox)
				.IsChecked(ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					CurrentFilterOptions.bIncludeGameWorld = (NewState == ECheckBoxState::Checked);
					OnFilterOptionChanged();
				})
				[
					SNew(STextBlock)
					.Text(LOCTEXT("IncludeGameWorld", "包含游戏世界"))
				]
			]

			// 第二行：显示选项
			+ SGridPanel::Slot(0, 1)
			.Padding(5)
			[
				SAssignNew(ShowMemoryInfoCheckBox, SCheckBox)
				.IsChecked(ECheckBoxState::Checked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					CurrentFilterOptions.bShowMemoryInfo = (NewState == ECheckBoxState::Checked);
					OnFilterOptionChanged();
				})
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ShowMemoryInfo", "显示内存信息"))
				]
			]

			+ SGridPanel::Slot(1, 1)
			.Padding(5)
			[
				SAssignNew(ShowReferenceChainCheckBox, SCheckBox)
				.IsChecked(ECheckBoxState::Checked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					CurrentFilterOptions.bShowReferenceChain = (NewState == ECheckBoxState::Checked);
					OnFilterOptionChanged();
				})
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ShowReferenceChain", "显示引用链"))
				]
			]

			+ SGridPanel::Slot(2, 1)
			.Padding(5)
			[
				SAssignNew(ShowStatisticsCheckBox, SCheckBox)
				.IsChecked(ECheckBoxState::Checked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					CurrentFilterOptions.bShowStatistics = (NewState == ECheckBoxState::Checked);
					OnFilterOptionChanged();
				})
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ShowStatistics", "显示统计信息"))
				]
			]

			// 第三行：自动刷新
			+ SGridPanel::Slot(0, 2)
			.Padding(5)
			[
				SAssignNew(AutoRefreshCheckBox, SCheckBox)
				.IsChecked(ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					CurrentFilterOptions.bAutoRefresh = (NewState == ECheckBoxState::Checked);
					OnFilterOptionChanged();
				})
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AutoRefresh", "自动刷新"))
				]
			]
		];
}

TSharedRef<SWidget> SObjRefDebuggerWindow::BuildStatisticsPanel()
{
	return SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(10.0f)
		.Visibility_Lambda([this]()
		{
			return CurrentFilterOptions.bShowStatistics ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("StatisticsTitle", "搜索统计"))
				.Font(FEditorStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SGridPanel)
				.FillColumn(0, 1.0f)
				.FillColumn(1, 1.0f)
				.FillColumn(2, 1.0f)

				// 第一行统计
				+ SGridPanel::Slot(0, 0)
				.Padding(5)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TotalInstances", "总实例数"))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(TotalInstancesText, STextBlock)
						.Text(FText::FromString(TEXT("0")))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
					]
				]

				+ SGridPanel::Slot(1, 0)
				.Padding(5)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TotalReferencers", "总引用者数"))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(TotalReferencersText, STextBlock)
						.Text(FText::FromString(TEXT("0")))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
					]
				]

				+ SGridPanel::Slot(2, 0)
				.Padding(5)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("GCRootCount", "GC根对象"))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(GCRootCountText, STextBlock)
						.Text(FText::FromString(TEXT("0")))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
					]
				]

				// 第二行统计
				+ SGridPanel::Slot(0, 1)
				.Padding(5)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MemoryUsage", "内存使用"))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(MemoryUsageText, STextBlock)
						.Text(FText::FromString(TEXT("0 KB")))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
					]
				]

				+ SGridPanel::Slot(1, 1)
				.Padding(5)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SearchDuration", "搜索耗时"))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(SearchDurationText, STextBlock)
						.Text(FText::FromString(TEXT("0.0s")))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
					]
				]

				+ SGridPanel::Slot(2, 1)
				.Padding(5)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MaxDepth", "最大深度"))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(MaxDepthText, STextBlock)
						.Text(FText::FromString(TEXT("0")))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
					]
				]
			]
		];
}

// === 缺失的行生成函数 ===

TSharedRef<ITableRow> SObjRefDebuggerWindow::OnGenerateObjectRow(TSharedPtr<FObjectListItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FObjectListItem>>, OwnerTable)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->ObjectName))
					.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("类: %s"), *Item->ClassName)))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(10, 0, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("世界: %s"), *Item->WorldName)))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(10, 0, 0, 0)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Item]()
						{
							return CurrentFilterOptions.bShowMemoryInfo ?
								FText::FromString(FString::Printf(TEXT("内存: %d 字节"), Item->MemorySize)) :
								FText::GetEmpty();
						})
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Visibility_Lambda([this]()
						{
							return CurrentFilterOptions.bShowMemoryInfo ? EVisibility::Visible : EVisibility::Collapsed;
						})
					]
				]
			]
		];
}

TSharedRef<ITableRow> SObjRefDebuggerWindow::OnGenerateReferencerRow(TSharedPtr<FReferencerInfo> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FLinearColor ReferenceColor = Item->bIsStrongReference ? 
		FDebuggerTheme::Get().StrongReferenceColor : 
		FDebuggerTheme::Get().WeakReferenceColor;

	return SNew(STableRow<TSharedPtr<FReferencerInfo>>, OwnerTable)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->bIsStrongReference ? TEXT("强") : TEXT("弱")))
				.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
				.ColorAndOpacity(ReferenceColor)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(5.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->ReferencerName))
					.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("类: %s"), *Item->ReferencerClass)))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(10, 0, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("属性: %s"), *Item->PropertyName)))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(10, 0, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("深度: %d"), Item->ReferenceDepth)))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
		];
}

TSharedRef<ITableRow> SObjRefDebuggerWindow::OnGenerateReferenceChainRow(TSharedPtr<FReferenceChainNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	FLinearColor RowColor = FLinearColor::White;
	
	// 根据深度和类型调整颜色
	if (Item->bIsGCRoot)
	{
		RowColor = FDebuggerTheme::Get().GCRootColor;
	}
	else if (Item->Depth == 1)
	{
		RowColor = FDebuggerTheme::Get().DirectReferenceColor;
	}
	else
	{
		RowColor = FDebuggerTheme::Get().IndirectReferenceColor;
	}

	return SNew(STableRow<TSharedPtr<FReferenceChainNode>>, OwnerTable)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("L%d"), Item->Depth)))
				.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
				.ColorAndOpacity(RowColor)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(5.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->ObjectName))
					.Font(FEditorStyle::GetFontStyle("PropertyWindow.BoldFont"))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("类: %s"), *Item->ClassName)))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(10, 0, 0, 0)
					[
						SNew(STextBlock)
						.Text_Lambda([Item]()
						{
							return Item->PropertyName.IsEmpty() ? 
								FText::GetEmpty() : 
								FText::FromString(FString::Printf(TEXT("通过: %s"), *Item->PropertyName));
						})
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
		];
}

// === 基础事件处理 ===

FReply SObjRefDebuggerWindow::OnSearchClicked()
{
	UE_LOG(LogTemp, Log, TEXT("OnSearchClicked 被调用，当前类数量: %d, 正在搜索: %s"), 
		CurrentClassNames.Num(), bIsSearching ? TEXT("是") : TEXT("否"));
		
	if (CurrentClassNames.Num() > 0 && !bIsSearching)
	{
		// 生成缓存键
		TArray<FString> ClassNameStrings;
		for (const TSharedPtr<FString>& ClassName : CurrentClassNames)
		{
			if (ClassName.IsValid())
			{
				ClassNameStrings.Add(*ClassName);
				UE_LOG(LogTemp, Log, TEXT("准备搜索类: %s"), **ClassName);
			}
		}
		FString CacheKey = FString::Join(ClassNameStrings, TEXT(","));
		UE_LOG(LogTemp, Log, TEXT("生成的缓存键: %s"), *CacheKey);
		
		// 检查缓存
		if (CachedSearchResults.Contains(CacheKey))
		{
			UE_LOG(LogTemp, Log, TEXT("使用缓存结果"));
			ObjectInstances = CachedSearchResults[CacheKey];
			UE_LOG(LogTemp, Log, TEXT("缓存结果包含 %d 个对象实例"), ObjectInstances.Num());
			
			if (ObjectListView.IsValid())
			{
				ObjectListView->RequestListRefresh();
				UE_LOG(LogTemp, Log, TEXT("ObjectListView 刷新完成"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ObjectListView 无效"));
			}
			
			CalculateStatistics();
			UpdateStatisticsDisplay();
			UE_LOG(LogTemp, Log, TEXT("统计信息和显示已更新"));
			return FReply::Handled();
		}

		// 启动异步搜索
		UE_LOG(LogTemp, Log, TEXT("启动多类搜索"));
		StartAsyncMultiClassSearch(ClassNameStrings);
	}
	else
	{
		if (CurrentClassNames.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("没有选择任何类"));
		}
		if (bIsSearching)
		{
			UE_LOG(LogTemp, Warning, TEXT("当前正在搜索中"));
		}
	}
	
	return FReply::Handled();
}

void SObjRefDebuggerWindow::OnClassSelected(UClass* SelectedClass)
{
	if (SelectedClass)
	{
		FString ClassName = SelectedClass->GetName();
		UE_LOG(LogTemp, Log, TEXT("选择了类: %s"), *ClassName);
		
		// 检查是否已经选择过这个类
		bool bAlreadySelected = false;
		for (const TSharedPtr<FString>& ExistingClassName : CurrentClassNames)
		{
			if (ExistingClassName.IsValid() && *ExistingClassName == ClassName)
			{
				bAlreadySelected = true;
				UE_LOG(LogTemp, Log, TEXT("类 %s 已经存在于列表中"), *ClassName);
				break;
			}
		}
		
		// 如果没有选择过，则添加到列表
		if (!bAlreadySelected)
		{
			CurrentClassNames.Add(MakeShareable(new FString(ClassName)));
			UE_LOG(LogTemp, Log, TEXT("添加类 %s 到列表，当前列表大小: %d"), *ClassName, CurrentClassNames.Num());
			
			// 刷新列表显示
			if (SelectedClassListView.IsValid())
			{
				SelectedClassListView->RequestListRefresh();
				UE_LOG(LogTemp, Log, TEXT("刷新已选择类列表显示"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SelectedClassListView 无效"));
			}
			
			// 不自动搜索，等用户手动点击搜索按钮
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("选择的类为空"));
	}
}

void SObjRefDebuggerWindow::OnObjectSelectionChanged(TSharedPtr<FObjectListItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	CurrentSelectedObject = SelectedItem;
	ReferencerInfos.Empty();
	ReferenceChainRoots.Empty();
	
	if (SelectedItem.IsValid() && IsValid(SelectedItem->Object))
	{
		UObject* TargetObject = SelectedItem->Object;
		UE_LOG(LogTemp, Log, TEXT("选择了对象: %s"), *TargetObject->GetName());
		
		// 检查引用者缓存
		if (CachedReferencers.Contains(TargetObject))
		{
			UE_LOG(LogTemp, Log, TEXT("使用缓存的引用者信息"));
			ReferencerInfos = CachedReferencers[TargetObject];
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("查找对象引用者"));
			// 查找引用者
			FindObjectReferencers(TargetObject, ReferencerInfos);
			// 缓存结果
			CachedReferencers.Add(TargetObject, ReferencerInfos);
		}
		
		// 构建引用链（如果启用）
		if (CurrentFilterOptions.bShowReferenceChain)
		{
			// 检查引用链缓存
			if (CachedReferenceChains.Contains(TargetObject))
			{
				UE_LOG(LogTemp, Log, TEXT("使用缓存的引用链信息"));
				ReferenceChainRoots = CachedReferenceChains[TargetObject];
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("构建引用链到GC根"));
				BuildReferenceChainToRoot(TargetObject, ReferenceChainRoots);
				// 缓存结果
				CachedReferenceChains.Add(TargetObject, ReferenceChainRoots);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("未选择有效对象"));
	}
	
	ReferencerListView->RequestListRefresh();
	ReferenceChainTreeView->RequestTreeRefresh();
}

void SObjRefDebuggerWindow::OnReferenceChainSelectionChanged(TSharedPtr<FReferenceChainNode> SelectedItem, ESelectInfo::Type SelectInfo)
{
	// 可以添加选中引用链节点的附加逻辑
}

void SObjRefDebuggerWindow::OnFilterOptionChanged()
{
	// 当过滤选项改变时，如果有缓存的结果则重新过滤
	if (CurrentClassNames.Num() > 0)
	{
		TArray<FString> ClassNameStrings;
		for (const TSharedPtr<FString>& ClassName : CurrentClassNames)
		{
			if (ClassName.IsValid())
			{
				ClassNameStrings.Add(*ClassName);
			}
		}
		FString CacheKey = FString::Join(ClassNameStrings, TEXT(","));
		
		if (CachedSearchResults.Contains(CacheKey))
		{
			// 重新应用过滤器
			OnSearchClicked();
		}
	}
}

void SObjRefDebuggerWindow::OnGetReferenceChainChildren(TSharedPtr<FReferenceChainNode> Item, TArray<TSharedPtr<FReferenceChainNode>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren = Item->Children;
	}
}

TSharedRef<ITableRow> SObjRefDebuggerWindow::OnGenerateSelectedClassRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!Item.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnGenerateSelectedClassRow: Item 无效"));
		return SNew(STableRow<TSharedPtr<FString>>, OwnerTable);
	}
	
	FString ClassName = *Item;
	UE_LOG(LogTemp, Log, TEXT("生成已选择类行: %s"), *ClassName);
	
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		[
			SNew(SHorizontalBox)
			
			// 类名
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(5, 2)
			[
				SNew(STextBlock)
				.Text(FText::FromString(ClassName))
				.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
			]
			
			// 移除按钮
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(5, 2)
			[
				SNew(SButton)
				.ButtonStyle(FEditorStyle::Get(), "FlatButton")
				.Text(LOCTEXT("RemoveClass", "移除"))
				.OnClicked_Lambda([this, ClassName]()
				{
					UE_LOG(LogTemp, Log, TEXT("移除按钮被点击: %s"), *ClassName);
					RemoveSelectedClass(ClassName);
					return FReply::Handled();
				})
				.ToolTipText(LOCTEXT("RemoveClassTooltip", "从搜索列表中移除这个类"))
			]
		];
}

void SObjRefDebuggerWindow::RemoveSelectedClass(const FString& ClassName)
{
	UE_LOG(LogTemp, Log, TEXT("尝试移除类: %s"), *ClassName);
	
	// 从列表中移除指定的类
	int32 RemovedCount = 0;
	for (int32 i = CurrentClassNames.Num() - 1; i >= 0; --i)
	{
		if (CurrentClassNames[i].IsValid() && *CurrentClassNames[i] == ClassName)
		{
			CurrentClassNames.RemoveAt(i);
			RemovedCount++;
			UE_LOG(LogTemp, Log, TEXT("成功移除类: %s"), *ClassName);
			break;
		}
	}
	
	if (RemovedCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("未找到要移除的类: %s"), *ClassName);
	}
	
	UE_LOG(LogTemp, Log, TEXT("移除后，当前类列表大小: %d"), CurrentClassNames.Num());
	
	// 刷新列表显示
	if (SelectedClassListView.IsValid())
	{
		SelectedClassListView->RequestListRefresh();
		UE_LOG(LogTemp, Log, TEXT("刷新已选择类列表显示"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectedClassListView 无效"));
	}
	
	// 如果没有类了，清除搜索结果，但不自动重新搜索
	if (CurrentClassNames.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("没有类了，清除搜索结果"));
		OnClearResultsClicked();
	}
	// 如果还有类，等用户手动点击搜索按钮
}

#undef LOCTEXT_NAMESPACE 