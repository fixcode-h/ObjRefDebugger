// Copyright Epic Games, Inc. All Rights Reserved.

#include "SObjRefDebuggerWindow.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "EditorDirectories.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "SObjRefDebuggerWindow"

TSharedRef<SWidget> SObjRefDebuggerWindow::BuildMainContent()
{
	return SNew(SSplitter)
		.Orientation(Orient_Horizontal)

		// 左侧：对象实例列表
		+ SSplitter::Slot()
		.Value(0.25f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(5.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 5)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() 
					{
						return FText::FromString(FString::Printf(TEXT("类实例列表 (%d)"), ObjectInstances.Num()));
					})
					.Font(FEditorStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(ObjectListView, SListView<TSharedPtr<FObjectListItem>>)
					.ListItemsSource(&ObjectInstances)
					.OnGenerateRow(this, &SObjRefDebuggerWindow::OnGenerateObjectRow)
					.OnSelectionChanged(this, &SObjRefDebuggerWindow::OnObjectSelectionChanged)
					.SelectionMode(ESelectionMode::Single)
				]
			]
		]

		// 中间左：引用者详情
		+ SSplitter::Slot()
		.Value(0.25f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(5.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 5)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::FromString(FString::Printf(TEXT("引用者详情 (%d)"), ReferencerInfos.Num()));
					})
					.Font(FEditorStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(ReferencerListView, SListView<TSharedPtr<FReferencerInfo>>)
					.ListItemsSource(&ReferencerInfos)
					.OnGenerateRow(this, &SObjRefDebuggerWindow::OnGenerateReferencerRow)
					.SelectionMode(ESelectionMode::Single)
				]
			]
		]

		// 中间右：引用链可视化
		+ SSplitter::Slot()
		.Value(0.25f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(5.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 5)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ReferenceChainTitle", "引用链到GC根"))
					.Font(FEditorStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(ReferenceChainTreeView, STreeView<TSharedPtr<FReferenceChainNode>>)
					.TreeItemsSource(&ReferenceChainRoots)
					.OnGenerateRow(this, &SObjRefDebuggerWindow::OnGenerateReferenceChainRow)
					.OnGetChildren(this, &SObjRefDebuggerWindow::OnGetReferenceChainChildren)
					.OnSelectionChanged(this, &SObjRefDebuggerWindow::OnReferenceChainSelectionChanged)
					.SelectionMode(ESelectionMode::Single)
				]
			]
		]

		// 右侧：搜索历史
		+ SSplitter::Slot()
		.Value(0.25f)
		[
			BuildSearchHistoryPanel()
		];
}

TSharedRef<SWidget> SObjRefDebuggerWindow::BuildSearchHistoryPanel()
{
	return SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(5.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::FromString(FString::Printf(TEXT("搜索历史 (%d)"), SearchHistory.Num()));
					})
					.Font(FEditorStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("ClearHistory", "清除"))
					.OnClicked_Lambda([this]()
					{
						SearchHistory.Empty();
						SearchHistoryListView->RequestListRefresh();
						return FReply::Handled();
					})
					.ToolTipText(LOCTEXT("ClearHistoryTooltip", "清除搜索历史"))
				]
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(SearchHistoryListView, SListView<TSharedPtr<FSearchHistoryItem>>)
				.ListItemsSource(&SearchHistory)
				.OnGenerateRow(this, &SObjRefDebuggerWindow::OnGenerateSearchHistoryRow)
				.OnSelectionChanged(this, &SObjRefDebuggerWindow::OnSearchHistorySelectionChanged)
				.SelectionMode(ESelectionMode::Single)
			]
		];
}

// === 事件处理函数 ===

FReply SObjRefDebuggerWindow::OnForceGCClicked()
{
	// 执行强制垃圾回收
	PerformForceGC();
	return FReply::Handled();
}

FReply SObjRefDebuggerWindow::OnClearResultsClicked()
{
	ObjectInstances.Empty();
	ReferencerInfos.Empty();
	ReferenceChainRoots.Empty();
	CurrentStatistics.Reset();
	
	// 清除所有缓存
	CachedSearchResults.Empty();
	CachedReferencers.Empty();
	CachedReferenceChains.Empty();
	
	ObjectListView->RequestListRefresh();
	ReferencerListView->RequestListRefresh();
	ReferenceChainTreeView->RequestTreeRefresh();
	UpdateStatisticsDisplay();
	
	UE_LOG(LogTemp, Log, TEXT("清除了所有结果和缓存"));
	
	return FReply::Handled();
}

FReply SObjRefDebuggerWindow::OnRefreshClicked()
{
	// OnSearchClicked 现在已经包含清除缓存的逻辑，直接调用即可
	return OnSearchClicked();
}

FReply SObjRefDebuggerWindow::OnExportResultsClicked()
{
	if (ObjectInstances.Num() == 0)
	{
		// 显示通知
		FNotificationInfo Info(LOCTEXT("NoDataToExport", "没有数据可导出"));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return FReply::Handled();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> SaveFilenames;
		bool bSaved = DesktopPlatform->SaveFileDialog(
			nullptr,
			LOCTEXT("ExportResults", "导出搜索结果").ToString(),
			FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
			FString::Printf(TEXT("%s_ObjectReferences_%s"), 
				*FString::Join(TArray<FString>([this]() {
					TArray<FString> Result;
					for (const TSharedPtr<FString>& ClassName : CurrentClassNames)
					{
						if (ClassName.IsValid()) Result.Add(*ClassName);
					}
					return Result;
				}()), TEXT("_")), 
				*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))),
			TEXT("CSV Files (*.csv)|*.csv|JSON Files (*.json)|*.json"),
			EFileDialogFlags::None,
			SaveFilenames
		);

		if (bSaved && SaveFilenames.Num() > 0)
		{
			FString FilePath = SaveFilenames[0];
			FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, FPaths::GetPath(FilePath));

			if (FilePath.EndsWith(TEXT(".csv")))
			{
				ExportToCSV(FilePath);
			}
			else if (FilePath.EndsWith(TEXT(".json")))
			{
				ExportToJSON(FilePath);
			}

			// 显示成功通知
			FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("成功导出到: %s"), *FilePath)));
			Info.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}

	return FReply::Handled();
}

void SObjRefDebuggerWindow::OnSearchHistorySelectionChanged(TSharedPtr<FSearchHistoryItem> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid() && SelectInfo == ESelectInfo::OnMouseClick)
	{
		// 从历史记录中恢复搜索（历史记录目前只支持单个类）
		CurrentClassNames.Empty();
		CurrentClassNames.Add(MakeShareable(new FString(SelectedItem->ClassName)));
		
		// 刷新已选择类列表显示
		if (SelectedClassListView.IsValid())
		{
			SelectedClassListView->RequestListRefresh();
		}
		
		// 在类选择器中查找对应的类
		UClass* TargetClass = nullptr;
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* Class = *ClassIt;
			if (Class->GetName() == SelectedItem->ClassName)
			{
				TargetClass = Class;
				break;
			}
		}
		
		if (TargetClass && ClassPicker.IsValid())
		{
			ClassPicker->SetSelectedClass(TargetClass);
		}
		
		OnSearchClicked();
	}
}

void SObjRefDebuggerWindow::OnViewModeChanged(int32 NewIndex)
{
	CurrentViewMode = NewIndex;
	// 这里可以根据视图模式切换不同的显示布局
}

void SObjRefDebuggerWindow::OnViewModeSelectionChanged(TSharedPtr<FText> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid())
	{
		// 找到选中项的索引
		for (int32 i = 0; i < ViewModeOptions.Num(); ++i)
		{
			if (ViewModeOptions[i] == SelectedItem)
			{
				OnViewModeChanged(i);
				break;
			}
		}
	}
}

FText SObjRefDebuggerWindow::GetCurrentViewModeText() const
{
	if (ViewModeOptions.IsValidIndex(CurrentViewMode))
	{
		return *ViewModeOptions[CurrentViewMode];
	}
	return LOCTEXT("DefaultViewMode", "列表视图");
}

// === 统计功能 ===

void SObjRefDebuggerWindow::CalculateStatistics()
{
	CurrentStatistics.Reset();
	
	CurrentStatistics.TotalInstancesFound = ObjectInstances.Num();
	CurrentStatistics.SearchDuration = LastSearchTime;

	// 计算总内存使用
	for (const auto& Item : ObjectInstances)
	{
		if (Item.IsValid())
		{
			CurrentStatistics.TotalMemoryUsage += Item->MemorySize;
			
			// 统计引用者数量
			CurrentStatistics.TotalReferencersFound += Item->ReferenceCount;
			
			// 统计GC根对象
			if (Item->bIsGCRoot)
			{
				CurrentStatistics.GCRootObjects++;
			}

			// 统计类分布
			CurrentStatistics.ClassDistribution.FindOrAdd(Item->ClassName)++;
			
			// 统计世界分布
			CurrentStatistics.WorldDistribution.FindOrAdd(Item->WorldName)++;
		}
	}

	// 计算最大引用深度
	for (const auto& RefInfo : ReferencerInfos)
	{
		if (RefInfo.IsValid())
		{
			CurrentStatistics.MaxReferenceDepth = FMath::Max(CurrentStatistics.MaxReferenceDepth, RefInfo->ReferenceDepth);
		}
	}

	// 统计无引用者的对象
	for (const auto& Item : ObjectInstances)
	{
		if (Item.IsValid() && Item->ReferenceCount == 0)
		{
			CurrentStatistics.ObjectsWithoutReferencers++;
		}
	}

	// 找出最常见的引用者类型
	FString MostCommonClass;
	int32 MaxCount = 0;
	for (const auto& Pair : CurrentStatistics.ClassDistribution)
	{
		if (Pair.Value > MaxCount)
		{
			MaxCount = Pair.Value;
			MostCommonClass = Pair.Key;
		}
	}
	CurrentStatistics.MostCommonReferencer = MostCommonClass;
}

void SObjRefDebuggerWindow::UpdateStatisticsDisplay()
{
	if (TotalInstancesText.IsValid())
	{
		TotalInstancesText->SetText(FText::FromString(FString::Printf(TEXT("%d"), CurrentStatistics.TotalInstancesFound)));
	}
	
	if (TotalReferencersText.IsValid())
	{
		TotalReferencersText->SetText(FText::FromString(FString::Printf(TEXT("%d"), CurrentStatistics.TotalReferencersFound)));
	}
	
	if (GCRootCountText.IsValid())
	{
		GCRootCountText->SetText(FText::FromString(FString::Printf(TEXT("%d"), CurrentStatistics.GCRootObjects)));
	}
	
	if (MemoryUsageText.IsValid())
	{
		float MemoryKB = CurrentStatistics.TotalMemoryUsage / 1024.0f;
		FString MemoryStr;
		if (MemoryKB > 1024.0f)
		{
			MemoryStr = FString::Printf(TEXT("%.2f MB"), MemoryKB / 1024.0f);
		}
		else
		{
			MemoryStr = FString::Printf(TEXT("%.2f KB"), MemoryKB);
		}
		MemoryUsageText->SetText(FText::FromString(MemoryStr));
	}
	
	if (SearchDurationText.IsValid())
	{
		SearchDurationText->SetText(FText::FromString(FString::Printf(TEXT("%.3fs"), CurrentStatistics.SearchDuration)));
	}
	
	if (MaxDepthText.IsValid())
	{
		MaxDepthText->SetText(FText::FromString(FString::Printf(TEXT("%d"), CurrentStatistics.MaxReferenceDepth)));
	}
}

void SObjRefDebuggerWindow::AddToSearchHistory(const FString& ClassName, int32 ResultCount, float SearchDuration)
{
	// 检查是否已存在相同的搜索
	for (int32 i = SearchHistory.Num() - 1; i >= 0; --i)
	{
		if (SearchHistory[i]->ClassName == ClassName)
		{
			SearchHistory.RemoveAt(i);
			break;
		}
	}

	// 添加新的搜索记录
	SearchHistory.Insert(MakeShareable(new FSearchHistoryItem(ClassName, ResultCount, SearchDuration)), 0);

	// 限制历史记录数量
	const int32 MaxHistoryItems = 20;
	while (SearchHistory.Num() > MaxHistoryItems)
	{
		SearchHistory.RemoveAt(SearchHistory.Num() - 1);
	}

	if (SearchHistoryListView.IsValid())
	{
		SearchHistoryListView->RequestListRefresh();
	}
}

// === 导出功能 ===

void SObjRefDebuggerWindow::ExportToCSV(const FString& FilePath)
{
	FString CSVContent;
	
	// CSV 标题行
	CSVContent += TEXT("对象名称,类名,世界,内存大小(字节),引用者数量,是否GC根\n");
	
	// 数据行
	for (const auto& Item : ObjectInstances)
	{
		if (Item.IsValid())
		{
			CSVContent += FString::Printf(TEXT("%s,%s,%s,%d,%d,%s\n"),
				*Item->ObjectName.Replace(TEXT(","), TEXT(";")),  // 替换逗号避免CSV格式问题
				*Item->ClassName,
				*Item->WorldName.Replace(TEXT(","), TEXT(";")),
				Item->MemorySize,
				Item->ReferenceCount,
				Item->bIsGCRoot ? TEXT("是") : TEXT("否")
			);
		}
	}
	
	// 添加统计信息
	CSVContent += TEXT("\n统计信息\n");
	CSVContent += FString::Printf(TEXT("总实例数,%d\n"), CurrentStatistics.TotalInstancesFound);
	CSVContent += FString::Printf(TEXT("总引用者数,%d\n"), CurrentStatistics.TotalReferencersFound);
	CSVContent += FString::Printf(TEXT("GC根对象数,%d\n"), CurrentStatistics.GCRootObjects);
	CSVContent += FString::Printf(TEXT("总内存使用,%.2f KB\n"), CurrentStatistics.TotalMemoryUsage / 1024.0f);
	CSVContent += FString::Printf(TEXT("搜索耗时,%.3f 秒\n"), CurrentStatistics.SearchDuration);
	
	FFileHelper::SaveStringToFile(CSVContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void SObjRefDebuggerWindow::ExportToJSON(const FString& FilePath)
{
	FString JSONContent = TEXT("{\n");
	
	// 生成类名数组字符串
	TArray<FString> ClassNameStrings;
	for (const TSharedPtr<FString>& ClassName : CurrentClassNames)
	{
		if (ClassName.IsValid())
		{
			ClassNameStrings.Add(FString::Printf(TEXT("\"%s\""), **ClassName));
		}
	}
	
	JSONContent += FString::Printf(TEXT("  \"searchClasses\": [%s],\n"), *FString::Join(ClassNameStrings, TEXT(", ")));
	JSONContent += FString::Printf(TEXT("  \"exportTime\": \"%s\",\n"), *FDateTime::Now().ToString());
	JSONContent += TEXT("  \"statistics\": {\n");
	JSONContent += FString::Printf(TEXT("    \"totalInstances\": %d,\n"), CurrentStatistics.TotalInstancesFound);
	JSONContent += FString::Printf(TEXT("    \"totalReferencers\": %d,\n"), CurrentStatistics.TotalReferencersFound);
	JSONContent += FString::Printf(TEXT("    \"gcRootObjects\": %d,\n"), CurrentStatistics.GCRootObjects);
	JSONContent += FString::Printf(TEXT("    \"memoryUsageBytes\": %.0f,\n"), CurrentStatistics.TotalMemoryUsage);
	JSONContent += FString::Printf(TEXT("    \"searchDurationSeconds\": %.3f\n"), CurrentStatistics.SearchDuration);
	JSONContent += TEXT("  },\n");
	
	JSONContent += TEXT("  \"instances\": [\n");
	for (int32 i = 0; i < ObjectInstances.Num(); ++i)
	{
		const auto& Item = ObjectInstances[i];
		if (Item.IsValid())
		{
			JSONContent += TEXT("    {\n");
			JSONContent += FString::Printf(TEXT("      \"name\": \"%s\",\n"), *Item->ObjectName.Replace(TEXT("\""), TEXT("\\\"")));
			JSONContent += FString::Printf(TEXT("      \"class\": \"%s\",\n"), *Item->ClassName);
			JSONContent += FString::Printf(TEXT("      \"world\": \"%s\",\n"), *Item->WorldName);
			JSONContent += FString::Printf(TEXT("      \"memorySize\": %d,\n"), Item->MemorySize);
			JSONContent += FString::Printf(TEXT("      \"referenceCount\": %d,\n"), Item->ReferenceCount);
			JSONContent += FString::Printf(TEXT("      \"isGCRoot\": %s\n"), Item->bIsGCRoot ? TEXT("true") : TEXT("false"));
			JSONContent += TEXT("    }");
			if (i < ObjectInstances.Num() - 1) JSONContent += TEXT(",");
			JSONContent += TEXT("\n");
		}
	}
	JSONContent += TEXT("  ]\n");
	JSONContent += TEXT("}\n");
	
	FFileHelper::SaveStringToFile(JSONContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

TSharedRef<ITableRow> SObjRefDebuggerWindow::OnGenerateSearchHistoryRow(TSharedPtr<FSearchHistoryItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FSearchHistoryItem>>, OwnerTable)
		[
			SNew(SHorizontalBox)

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
					.Text(FText::FromString(Item->ClassName))
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
						.Text(FText::FromString(FString::Printf(TEXT("结果: %d"), Item->ResultCount)))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(10, 0, 0, 0)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("耗时: %.3fs"), Item->SearchDuration)))
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->SearchTime.ToString(TEXT("%m/%d %H:%M:%S"))))
					.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		];
}

#undef LOCTEXT_NAMESPACE 