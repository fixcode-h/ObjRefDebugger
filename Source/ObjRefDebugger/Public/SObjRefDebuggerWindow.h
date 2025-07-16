// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STreeView.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "ObjRefDebuggerTypes.h"
#include "SObjRefDebuggerClassPicker.h"

class SEditableTextBox;
class SButton;
class SSplitter;
class SCheckBox;
class SProgressBar;
class STextBlock;

/** 主调试器窗口类 */
class SObjRefDebuggerWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SObjRefDebuggerWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// === UI 事件处理 ===
	
	/** 搜索按钮点击事件 */
	FReply OnSearchClicked();
	
	/** 清除结果按钮点击事件 */
	FReply OnClearResultsClicked();
	
	/** 刷新按钮点击事件 */
	FReply OnRefreshClicked();
	
	/** 导出结果按钮点击事件 */
	FReply OnExportResultsClicked();
	
	/** 强制GC按钮点击事件 */
	FReply OnForceGCClicked();
	
	/** 类选择改变事件 */
	void OnClassSelected(UClass* SelectedClass);
	
	/** 移除选中的类 */
	void RemoveSelectedClass(const FString& ClassName);
	
	/** 对象实例列表选择改变事件 */
	void OnObjectSelectionChanged(TSharedPtr<FObjectListItem> SelectedItem, ESelectInfo::Type SelectInfo);
	
	/** 引用链树节点选择改变事件 */
	void OnReferenceChainSelectionChanged(TSharedPtr<FReferenceChainNode> SelectedItem, ESelectInfo::Type SelectInfo);
	
	/** 搜索历史选择改变事件 */
	void OnSearchHistorySelectionChanged(TSharedPtr<FSearchHistoryItem> SelectedItem, ESelectInfo::Type SelectInfo);
	
	/** 过滤选项改变事件 */
	void OnFilterOptionChanged();
	
	/** 视图模式切换事件 */
	void OnViewModeChanged(int32 NewIndex);
	
	/** 视图模式选择变化事件 */
	void OnViewModeSelectionChanged(TSharedPtr<FText> SelectedItem, ESelectInfo::Type SelectInfo);
	
	/** 获取当前视图模式文本 */
	FText GetCurrentViewModeText() const;

	// === UI 生成器 ===
	
	/** 生成对象列表行 */
	TSharedRef<ITableRow> OnGenerateObjectRow(TSharedPtr<FObjectListItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	
	/** 生成引用者列表行 */
	TSharedRef<ITableRow> OnGenerateReferencerRow(TSharedPtr<FReferencerInfo> Item, const TSharedRef<STableViewBase>& OwnerTable);
	
	/** 生成引用链树节点 */
	TSharedRef<ITableRow> OnGenerateReferenceChainRow(TSharedPtr<FReferenceChainNode> Item, const TSharedRef<STableViewBase>& OwnerTable);
	
	/** 生成搜索历史行 */
	TSharedRef<ITableRow> OnGenerateSearchHistoryRow(TSharedPtr<FSearchHistoryItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	
	/** 生成已选择类行 */
	TSharedRef<ITableRow> OnGenerateSelectedClassRow(TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable);
	
	/** 获取引用链子节点 */
	void OnGetReferenceChainChildren(TSharedPtr<FReferenceChainNode> Item, TArray<TSharedPtr<FReferenceChainNode>>& OutChildren);

	// === UI 构建器 ===
	
	/** 构建顶部工具栏 */
	TSharedRef<SWidget> BuildTopToolbar();
	
	/** 构建搜索区域 */
	TSharedRef<SWidget> BuildSearchArea();
	
	/** 构建过滤选项区域 */
	TSharedRef<SWidget> BuildFilterOptions();
	
	/** 构建统计信息面板 */
	TSharedRef<SWidget> BuildStatisticsPanel();
	
	/** 构建主内容区域 */
	TSharedRef<SWidget> BuildMainContent();
	
	/** 构建搜索历史面板 */
	TSharedRef<SWidget> BuildSearchHistoryPanel();

	// === 核心功能 ===
	
	/** 查找指定类的所有实例 */
	void FindInstancesOfClass(const FString& ClassName, TArray<TSharedPtr<FObjectListItem>>& OutInstances);
	
	/** 查找对象的引用者 */
	void FindObjectReferencers(UObject* TargetObject, TArray<TSharedPtr<FReferencerInfo>>& OutReferencers);
	
	/** 构建引用链到GC根 */
	void BuildReferenceChainToRoot(UObject* TargetObject, TArray<TSharedPtr<FReferenceChainNode>>& OutRootNodes);
	
	/** 递归构建引用链 */
	void BuildReferenceChainRecursive(TSharedPtr<FReferenceChainNode> CurrentNode, TSet<UObject*>& VisitedObjects, int32 MaxDepth);
	
	/** 过滤对象（排除CDO、待销毁对象等） */
	bool ShouldIncludeObject(UObject* Object, const FSearchFilterOptions& FilterOptions) const;
	
	/** 获取指定世界类型的世界对象 */
	UWorld* GetWorldByType(EWorldType::Type WorldType) const;
	
	/** 计算搜索统计信息 */
	void CalculateStatistics();
	
	/** 更新统计显示 */
	void UpdateStatisticsDisplay();

	// === 性能优化 ===
	
	/** 异步搜索任务 */
	void StartAsyncSearch(const FString& ClassName);
	
	/** 异步多类搜索任务 */
	void StartAsyncMultiClassSearch(const TArray<FString>& ClassNames);
	
	/** 异步搜索完成回调 */
	void OnAsyncSearchComplete(TArray<TSharedPtr<FObjectListItem>> Results);
	
	/** 添加到搜索历史 */
	void AddToSearchHistory(const FString& ClassName, int32 ResultCount, float SearchDuration);

	// === GC 相关功能 ===
	
	/** 执行强制垃圾回收 */
	void PerformForceGC();
	
	/** GC 完成后的回调 */
	void OnGCComplete();

	// === 数据导出 ===
	
	/** 导出为CSV格式 */
	void ExportToCSV(const FString& FilePath);
	
	/** 导出为JSON格式 */
	void ExportToJSON(const FString& FilePath);

private:
	// === UI 控件 ===
	
	TSharedPtr<SObjRefDebuggerClassPicker> ClassPicker;
	TSharedPtr<SListView<TSharedPtr<FObjectListItem>>> ObjectListView;
	TSharedPtr<SListView<TSharedPtr<FReferencerInfo>>> ReferencerListView;
	TSharedPtr<STreeView<TSharedPtr<FReferenceChainNode>>> ReferenceChainTreeView;
	TSharedPtr<SListView<TSharedPtr<FSearchHistoryItem>>> SearchHistoryListView;
	TSharedPtr<SProgressBar> SearchProgressBar;
	TSharedPtr<SListView<TSharedPtr<FString>>> SelectedClassListView;
	
	// 过滤选项控件
	TSharedPtr<SCheckBox> IncludeEditorWorldCheckBox;
	TSharedPtr<SCheckBox> IncludePIEWorldCheckBox;
	TSharedPtr<SCheckBox> ShowMemoryInfoCheckBox;
	TSharedPtr<SCheckBox> ShowReferenceChainCheckBox;
	TSharedPtr<SCheckBox> ShowStatisticsCheckBox;
	TSharedPtr<SCheckBox> AutoRefreshCheckBox;
	
	// 统计信息显示控件
	TSharedPtr<STextBlock> TotalInstancesText;
	TSharedPtr<STextBlock> TotalReferencersText;
	TSharedPtr<STextBlock> MaxDepthText;
	TSharedPtr<STextBlock> GCRootCountText;
	TSharedPtr<STextBlock> MemoryUsageText;
	TSharedPtr<STextBlock> SearchDurationText;
	
	// 视图模式相关
	TArray<TSharedPtr<FText>> ViewModeOptions;
	
	// === 数据源 ===
	
	TArray<TSharedPtr<FObjectListItem>> ObjectInstances;
	TArray<TSharedPtr<FReferencerInfo>> ReferencerInfos;
	TArray<TSharedPtr<FReferenceChainNode>> ReferenceChainRoots;
	TArray<TSharedPtr<FSearchHistoryItem>> SearchHistory;
	
	// === 状态管理 ===
	
	TArray<TSharedPtr<FString>> CurrentClassNames;
	FSearchFilterOptions CurrentFilterOptions;
	FSearchStatistics CurrentStatistics;
	TSharedPtr<FObjectListItem> CurrentSelectedObject;
	
	// === 性能优化 ===
	
	bool bIsSearching;
	TMap<FString, TArray<TSharedPtr<FObjectListItem>>> CachedSearchResults;
	float LastSearchTime;
	FDateTime LastRefreshTime;
	
	// === UI 状态 ===
	
	int32 CurrentViewMode; // 0=列表模式, 1=详细模式, 2=图表模式
	bool bShowAdvancedOptions;
}; 