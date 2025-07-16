// Copyright Epic Games, Inc. All Rights Reserved.

#include "SObjRefDebuggerWindow.h"
#include "UObject/UObjectIterator.h"
#include "Engine/World.h"
#include "Editor.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Async/AsyncWork.h"
#include "Async/TaskGraphInterfaces.h"
#include "Async/Async.h"
#include "HAL/PlatformFilemanager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Containers/Ticker.h"

#define LOCTEXT_NAMESPACE "SObjRefDebuggerWindow"

// 异步搜索任务类
class FAsyncObjectSearchTask : public FNonAbandonableTask
{
public:
	FAsyncObjectSearchTask(const FString& InClassName, const FSearchFilterOptions& InFilterOptions)
		: ClassName(InClassName)
		, FilterOptions(InFilterOptions)
	{}

	void DoWork()
	{
		FindInstancesOfClassInternal(ClassName, FilterOptions, Results);
	}

	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncObjectSearchTask, STATGROUP_ThreadPoolAsyncTasks); }

	TArray<TSharedPtr<FObjectListItem>> GetResults() const { return Results; }

private:
	FString ClassName;
	FSearchFilterOptions FilterOptions;
	TArray<TSharedPtr<FObjectListItem>> Results;

	void FindInstancesOfClassInternal(const FString& InClassName, const FSearchFilterOptions& InFilterOptions, TArray<TSharedPtr<FObjectListItem>>& OutInstances)
	{
		// 首先查找指定的类
		UClass* TargetClass = nullptr;
		
		// 遍历所有UClass查找匹配的类名
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* CurrentClass = *ClassIt;
			if (CurrentClass->GetName() == InClassName)
			{
				TargetClass = CurrentClass;
				break;
			}
		}
		
		if (!TargetClass)
		{
			return;
		}

		// 获取相关世界对象
		TArray<UWorld*> WorldsToSearch;
		if (GEditor)
		{
			for (const FWorldContext& Context : GEditor->GetWorldContexts())
			{
				if ((Context.WorldType == EWorldType::Editor && InFilterOptions.bIncludeEditorWorld) ||
					(Context.WorldType == EWorldType::PIE && InFilterOptions.bIncludePIEWorld) ||
					(Context.WorldType == EWorldType::Game && InFilterOptions.bIncludeGameWorld))
				{
					if (Context.World())
					{
						WorldsToSearch.Add(Context.World());
					}
				}
			}
		}

		// 检查是否是AActor的子类
		if (TargetClass->IsChildOf(AActor::StaticClass()))
		{
			// 使用TActorIterator进行更安全的Actor查找
			for (UWorld* World : WorldsToSearch)
			{
				for (TActorIterator<AActor> ActorIt(World, TargetClass); ActorIt; ++ActorIt)
				{
					AActor* CurrentActor = *ActorIt;
					if (ShouldIncludeObjectStatic(CurrentActor, InFilterOptions))
					{
						OutInstances.Add(MakeShareable(new FObjectListItem(CurrentActor)));
					}
				}
			}
		}
		else
		{
			// 使用通用的TObjectIterator
			for (TObjectIterator<UObject> It; It; ++It)
			{
				UObject* CurrentObject = *It;
				if (CurrentObject->IsA(TargetClass))
				{
					if (ShouldIncludeObjectStatic(CurrentObject, InFilterOptions))
					{
						OutInstances.Add(MakeShareable(new FObjectListItem(CurrentObject)));
					}
				}
			}
		}
	}

	static bool ShouldIncludeObjectStatic(UObject* Object, const FSearchFilterOptions& FilterOptions)
	{
		if (!IsValid(Object))
		{
			return false;
		}

		// 过滤类默认对象 (CDO)
		if (Object->HasAnyFlags(RF_ClassDefaultObject))
		{
			return false;
		}

		// 过滤待销毁的对象
		if (Object->IsPendingKill())
		{
			return false;
		}

		// 过滤原型对象
		if (Object->HasAnyFlags(RF_ArchetypeObject))
		{
			return false;
		}

		// 世界过滤
		UWorld* ObjectWorld = Object->GetWorld();
		if (ObjectWorld)
		{
			EWorldType::Type WorldType = ObjectWorld->WorldType;
			if ((WorldType == EWorldType::Editor && !FilterOptions.bIncludeEditorWorld) ||
				(WorldType == EWorldType::PIE && !FilterOptions.bIncludePIEWorld) ||
				(WorldType == EWorldType::Game && !FilterOptions.bIncludeGameWorld))
			{
				return false;
			}
		}

		return true;
	}
};

void SObjRefDebuggerWindow::FindInstancesOfClass(const FString& ClassName, TArray<TSharedPtr<FObjectListItem>>& OutInstances)
{
	// 首先查找指定的类
	UClass* TargetClass = nullptr;
	
	// 遍历所有UClass查找匹配的类名
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* CurrentClass = *ClassIt;
		if (CurrentClass->GetName() == ClassName)
		{
			TargetClass = CurrentClass;
			break;
		}
	}
	
	if (!TargetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("找不到类: %s"), *ClassName);
		return;
	}

	// 获取相关世界对象
	TArray<UWorld*> WorldsToSearch;
	if (GEditor)
	{
		for (const FWorldContext& Context : GEditor->GetWorldContexts())
		{
			if ((Context.WorldType == EWorldType::Editor && CurrentFilterOptions.bIncludeEditorWorld) ||
				(Context.WorldType == EWorldType::PIE && CurrentFilterOptions.bIncludePIEWorld) ||
				(Context.WorldType == EWorldType::Game && CurrentFilterOptions.bIncludeGameWorld))
			{
				if (Context.World())
				{
					WorldsToSearch.Add(Context.World());
				}
			}
		}
	}

	// 检查是否是AActor的子类
	if (TargetClass->IsChildOf(AActor::StaticClass()))
	{
		// 使用TActorIterator进行更安全的Actor查找
		for (UWorld* World : WorldsToSearch)
		{
			for (TActorIterator<AActor> ActorIt(World, TargetClass); ActorIt; ++ActorIt)
			{
				AActor* CurrentActor = *ActorIt;
				if (ShouldIncludeObject(CurrentActor, CurrentFilterOptions))
				{
					OutInstances.Add(MakeShareable(new FObjectListItem(CurrentActor)));
				}
			}
		}
	}
	else
	{
		// 使用通用的TObjectIterator
		for (TObjectIterator<UObject> It; It; ++It)
		{
			UObject* CurrentObject = *It;
			if (CurrentObject->IsA(TargetClass))
			{
				if (ShouldIncludeObject(CurrentObject, CurrentFilterOptions))
				{
					OutInstances.Add(MakeShareable(new FObjectListItem(CurrentObject)));
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("找到 %d 个 %s 类的实例"), OutInstances.Num(), *ClassName);
}

void SObjRefDebuggerWindow::FindObjectReferencers(UObject* TargetObject, TArray<TSharedPtr<FReferencerInfo>>& OutReferencers)
{
	if (!IsValid(TargetObject))
	{
		return;
	}

	FReferencerInformationList ReferencerList;
	
	// 使用IsReferenced API查找引用者
	IsReferenced(TargetObject, RF_NoFlags, EInternalObjectFlags::AllFlags, true, &ReferencerList);

	OutReferencers.Empty();
	
	// 处理外部引用
	for (const FReferencerInformation& RefInfo : ReferencerList.ExternalReferences)
	{
		if (UObject* Referencer = RefInfo.Referencer)
		{
			FString PropName = TEXT("未知属性");
			
			// 获取引用属性信息
			if (RefInfo.ReferencingProperties.Num() > 0)
			{
				// 简化处理，取第一个属性
				const FProperty* Property = RefInfo.ReferencingProperties[0];
				PropName = Property->GetName();
			}

			FString ReferencerName = Referencer->GetName();
			FString ReferencerClass = Referencer->GetClass()->GetName();
			
			// 如果是世界中的对象，添加世界信息
			if (UWorld* ReferencerWorld = Referencer->GetWorld())
			{
				ReferencerName += FString::Printf(TEXT(" (%s)"), *ReferencerWorld->GetName());
			}

			OutReferencers.Add(MakeShareable(new FReferencerInfo(
				ReferencerName, 
				ReferencerClass, 
				PropName, 
				Referencer,
				0  // 初始深度为0
			)));
		}
	}
	
	// 处理内部引用
	for (const FReferencerInformation& RefInfo : ReferencerList.InternalReferences)
	{
		if (UObject* Referencer = RefInfo.Referencer)
		{
			FString PropName = TEXT("内部引用");
			
			if (RefInfo.ReferencingProperties.Num() > 0)
			{
				const FProperty* Property = RefInfo.ReferencingProperties[0];
				PropName = FString::Printf(TEXT("内部: %s"), *Property->GetName());
			}

			OutReferencers.Add(MakeShareable(new FReferencerInfo(
				Referencer->GetName(), 
				Referencer->GetClass()->GetName(), 
				PropName, 
				Referencer,
				0  // 内部引用深度为0
			)));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("为对象 %s 找到 %d 个引用者"), *TargetObject->GetName(), OutReferencers.Num());
}

void SObjRefDebuggerWindow::BuildReferenceChainToRoot(UObject* TargetObject, TArray<TSharedPtr<FReferenceChainNode>>& OutRootNodes)
{
	if (!IsValid(TargetObject))
	{
		return;
	}

	OutRootNodes.Empty();
	TSet<UObject*> VisitedObjects;

	// 创建目标对象的根节点
	TSharedPtr<FReferenceChainNode> TargetNode = MakeShareable(new FReferenceChainNode(TargetObject, 0));
	
	// 递归构建引用链
	BuildReferenceChainRecursive(TargetNode, VisitedObjects, CurrentFilterOptions.MaxReferenceDepth);

	// 添加到根节点列表
	OutRootNodes.Add(TargetNode);

	// 如果没有引用者，创建一个虚拟的GC根节点
	if (TargetNode->Children.Num() == 0)
	{
		TSharedPtr<FReferenceChainNode> GCRootNode = MakeShareable(new FReferenceChainNode(nullptr, 0));
		GCRootNode->ObjectName = TEXT("无引用者 - 可被GC回收");
		GCRootNode->ClassName = TEXT("GC状态");
		GCRootNode->Children.Add(TargetNode);
		TargetNode->Parent = GCRootNode;
		TargetNode->Depth = 1;
		
		OutRootNodes.Empty();
		OutRootNodes.Add(GCRootNode);
	}
}

void SObjRefDebuggerWindow::BuildReferenceChainRecursive(TSharedPtr<FReferenceChainNode> CurrentNode, TSet<UObject*>& VisitedObjects, int32 MaxDepth)
{
	if (!CurrentNode.IsValid() || !IsValid(CurrentNode->Object) || CurrentNode->Depth >= MaxDepth)
	{
		return;
	}

	// 防止循环引用
	if (VisitedObjects.Contains(CurrentNode->Object))
	{
		return;
	}
	VisitedObjects.Add(CurrentNode->Object);

	// 查找当前对象的引用者
	FReferencerInformationList ReferencerList;
	IsReferenced(CurrentNode->Object, RF_NoFlags, EInternalObjectFlags::AllFlags, true, &ReferencerList);

	// 处理外部引用
	for (const FReferencerInformation& RefInfo : ReferencerList.ExternalReferences)
	{
		if (UObject* Referencer = RefInfo.Referencer)
		{
			// 跳过已访问的对象
			if (VisitedObjects.Contains(Referencer))
			{
				continue;
			}

			TSharedPtr<FReferenceChainNode> ReferencerNode = MakeShareable(new FReferenceChainNode(Referencer, CurrentNode->Depth + 1));
			ReferencerNode->Parent = CurrentNode;
			
			// 设置引用属性名
			if (RefInfo.ReferencingProperties.Num() > 0)
			{
				const FProperty* Property = RefInfo.ReferencingProperties[0];
				ReferencerNode->PropertyName = Property->GetName();
			}

			CurrentNode->Children.Add(ReferencerNode);

			// 递归构建更深层的引用链
			BuildReferenceChainRecursive(ReferencerNode, VisitedObjects, MaxDepth);
		}
	}

	// 如果没有外部引用者，这可能是一个GC根
	if (CurrentNode->Children.Num() == 0 && CurrentNode->Depth > 0)
	{
		TSharedPtr<FReferenceChainNode> GCRootNode = MakeShareable(new FReferenceChainNode(nullptr, CurrentNode->Depth + 1));
		GCRootNode->ObjectName = TEXT("GC根");
		GCRootNode->ClassName = TEXT("Root");
		GCRootNode->Parent = CurrentNode;
		CurrentNode->Children.Add(GCRootNode);
	}

	VisitedObjects.Remove(CurrentNode->Object);
}

bool SObjRefDebuggerWindow::ShouldIncludeObject(UObject* Object, const FSearchFilterOptions& FilterOptions) const
{
	if (!IsValid(Object))
	{
		return false;
	}

	// 过滤类默认对象 (CDO)
	if (Object->HasAnyFlags(RF_ClassDefaultObject))
	{
		return false;
	}

	// 过滤待销毁的对象
	if (Object->IsPendingKill())
	{
		return false;
	}

	// 过滤原型对象
	if (Object->HasAnyFlags(RF_ArchetypeObject))
	{
		return false;
	}

	// 世界过滤
	UWorld* ObjectWorld = Object->GetWorld();
	if (ObjectWorld)
	{
		EWorldType::Type WorldType = ObjectWorld->WorldType;
		if ((WorldType == EWorldType::Editor && !FilterOptions.bIncludeEditorWorld) ||
			(WorldType == EWorldType::PIE && !FilterOptions.bIncludePIEWorld) ||
			(WorldType == EWorldType::Game && !FilterOptions.bIncludeGameWorld))
		{
			return false;
		}
	}

	return true;
}

UWorld* SObjRefDebuggerWindow::GetWorldByType(EWorldType::Type WorldType) const
{
	if (GEditor)
	{
		for (const FWorldContext& Context : GEditor->GetWorldContexts())
		{
			if (Context.WorldType == WorldType && Context.World())
			{
				return Context.World();
			}
		}
	}
	return nullptr;
}

void SObjRefDebuggerWindow::StartAsyncSearch(const FString& ClassName)
{
	bIsSearching = true;
	LastSearchTime = FPlatformTime::Seconds();

	// 使用简化的异步执行
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, ClassName]()
	{
		// 在后台线程执行搜索
		TArray<TSharedPtr<FObjectListItem>> Results;
		FAsyncObjectSearchTask SearchTask(ClassName, CurrentFilterOptions);
		SearchTask.DoWork();
		Results = SearchTask.GetResults();

		// 回到游戏线程更新UI
		AsyncTask(ENamedThreads::GameThread, [this, ClassName, Results]()
		{
			OnAsyncSearchComplete(Results);
			
			// 更新缓存
			CachedSearchResults.Add(ClassName, Results);
			bIsSearching = false;
		});
	});
}

void SObjRefDebuggerWindow::StartAsyncMultiClassSearch(const TArray<FString>& ClassNames)
{
	UE_LOG(LogTemp, Log, TEXT("StartAsyncMultiClassSearch 开始，要搜索 %d 个类"), ClassNames.Num());
	
	bIsSearching = true;
	LastSearchTime = FPlatformTime::Seconds();

	// 在主线程执行搜索（因为UE的迭代器不是线程安全的）
	TArray<TSharedPtr<FObjectListItem>> AllResults;
	
	for (const FString& ClassName : ClassNames)
	{
		UE_LOG(LogTemp, Log, TEXT("开始搜索类: %s"), *ClassName);
		
		// 首先查找指定的类
		UClass* TargetClass = nullptr;
		
		// 遍历所有UClass查找匹配的类名
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* CurrentClass = *ClassIt;
			if (CurrentClass->GetName() == ClassName)
			{
				TargetClass = CurrentClass;
				UE_LOG(LogTemp, Log, TEXT("找到目标类: %s"), *ClassName);
				break;
			}
		}
		
		if (!TargetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("找不到类: %s"), *ClassName);
			continue;
		}

		// 获取相关世界对象
		TArray<UWorld*> WorldsToSearch;
		if (GEditor)
		{
			for (const FWorldContext& Context : GEditor->GetWorldContexts())
			{
				if ((Context.WorldType == EWorldType::Editor && CurrentFilterOptions.bIncludeEditorWorld) ||
					(Context.WorldType == EWorldType::PIE && CurrentFilterOptions.bIncludePIEWorld) ||
					(Context.WorldType == EWorldType::Game && CurrentFilterOptions.bIncludeGameWorld))
				{
					if (Context.World())
					{
						WorldsToSearch.Add(Context.World());
					}
				}
			}
		}

		// 检查是否是AActor的子类
		if (TargetClass->IsChildOf(AActor::StaticClass()))
		{
			UE_LOG(LogTemp, Log, TEXT("类 %s 是 AActor 子类，使用 Actor 迭代器"), *ClassName);
			// 使用TActorIterator进行更安全的Actor查找
			for (UWorld* World : WorldsToSearch)
			{
				UE_LOG(LogTemp, Log, TEXT("在世界 %s 中搜索"), World ? *World->GetName() : TEXT("NULL"));
				int32 ActorCount = 0;
				for (TActorIterator<AActor> ActorIt(World, TargetClass); ActorIt; ++ActorIt)
				{
					AActor* CurrentActor = *ActorIt;
					if (ShouldIncludeObject(CurrentActor, CurrentFilterOptions))
					{
						AllResults.Add(MakeShareable(new FObjectListItem(CurrentActor)));
						ActorCount++;
					}
				}
				UE_LOG(LogTemp, Log, TEXT("在世界 %s 中找到 %d 个 %s 实例"), 
					World ? *World->GetName() : TEXT("NULL"), ActorCount, *ClassName);
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("类 %s 不是 AActor 子类，使用通用迭代器"), *ClassName);
			int32 ObjectCount = 0;
			// 使用通用的TObjectIterator
			for (TObjectIterator<UObject> It; It; ++It)
			{
				UObject* CurrentObject = *It;
				if (CurrentObject->IsA(TargetClass))
				{
					if (ShouldIncludeObject(CurrentObject, CurrentFilterOptions))
					{
						AllResults.Add(MakeShareable(new FObjectListItem(CurrentObject)));
						ObjectCount++;
					}
				}
			}
			UE_LOG(LogTemp, Log, TEXT("找到 %d 个 %s 实例"), ObjectCount, *ClassName);
		}
	}

	// 立即更新UI（因为我们在主线程执行）
	FString CacheKey = FString::Join(ClassNames, TEXT(","));
	UE_LOG(LogTemp, Log, TEXT("搜索完成，总共找到 %d 个实例，缓存键: %s"), AllResults.Num(), *CacheKey);
	
	OnAsyncSearchComplete(AllResults);
	
	// 更新缓存
	CachedSearchResults.Add(CacheKey, AllResults);
	UE_LOG(LogTemp, Log, TEXT("结果已缓存，缓存大小: %d"), CachedSearchResults.Num());
	
	bIsSearching = false;
	
	UE_LOG(LogTemp, Log, TEXT("多类搜索完成，找到 %d 个实例"), AllResults.Num());
}

void SObjRefDebuggerWindow::OnAsyncSearchComplete(TArray<TSharedPtr<FObjectListItem>> Results)
{
	ObjectInstances = Results;
	ReferencerInfos.Empty();
	ReferenceChainRoots.Empty();
	
	ObjectListView->RequestListRefresh();
	ReferencerListView->RequestListRefresh();
	ReferenceChainTreeView->RequestTreeRefresh();

	float SearchDuration = FPlatformTime::Seconds() - LastSearchTime;
	UE_LOG(LogTemp, Log, TEXT("异步搜索完成，用时 %.3f 秒，找到 %d 个实例"), SearchDuration, Results.Num());
}

// === GC 相关功能实现 ===

void SObjRefDebuggerWindow::PerformForceGC()
{
	// 显示通知
	FNotificationInfo Info(LOCTEXT("ForceGCStarted", "开始执行强制垃圾回收..."));
	Info.ExpireDuration = 2.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// 执行垃圾回收
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

	// 使用异步任务确保GC完全完成后再执行回调
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		// 延迟一帧确保GC完成
		FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float DeltaTime)
		{
			OnGCComplete();
			return false; // 只执行一次
		}), 0.5f);
	});
}

void SObjRefDebuggerWindow::OnGCComplete()
{
	// 显示GC完成通知
	FNotificationInfo Info(LOCTEXT("ForceGCCompleted", "垃圾回收完成，正在刷新搜索结果..."));
	Info.ExpireDuration = 2.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	// 如果当前有搜索结果，则自动刷新
	if (CurrentClassNames.Num() > 0)
	{
		// 清除缓存强制重新搜索
		TArray<FString> ClassNameStrings;
		for (const TSharedPtr<FString>& ClassName : CurrentClassNames)
		{
			if (ClassName.IsValid())
			{
				ClassNameStrings.Add(*ClassName);
			}
		}
		FString CacheKey = FString::Join(ClassNameStrings, TEXT(","));
		CachedSearchResults.Remove(CacheKey);
		
		// 执行搜索
		if (!bIsSearching)
		{
			StartAsyncMultiClassSearch(ClassNameStrings);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("强制GC完成，已自动刷新搜索结果"));
}

#undef LOCTEXT_NAMESPACE 