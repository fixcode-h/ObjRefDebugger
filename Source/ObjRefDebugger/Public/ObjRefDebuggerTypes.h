// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/** 对象列表项数据结构 */
struct FObjectListItem
{
	UObject* Object;
	FString ObjectName;
	FString ClassName;
	FString WorldName;
	int32 MemorySize;
	int32 ReferenceCount;  // 引用者数量
	bool bIsGCRoot;        // 是否为GC根
	
	FObjectListItem(UObject* InObject)
		: Object(InObject)
		, MemorySize(0)
		, ReferenceCount(0)
		, bIsGCRoot(false)
	{
		if (IsValid(InObject))
		{
			ObjectName = InObject->GetName();
			ClassName = InObject->GetClass()->GetName();
			UWorld* ObjectWorld = InObject->GetWorld();
			WorldName = ObjectWorld ? ObjectWorld->GetName() : TEXT("无世界");
			
			// 计算内存大小
			if (InObject->GetClass())
			{
				MemorySize = InObject->GetClass()->GetStructureSize();
			}
		}
		else
		{
			ObjectName = TEXT("Invalid Object");
			ClassName = TEXT("Unknown");
			WorldName = TEXT("Unknown");
		}
	}
};

/** 引用者信息数据结构 */
struct FReferencerInfo
{
	FString ReferencerName;
	FString ReferencerClass;
	FString PropertyName;
	UObject* ReferencerObject;
	int32 ReferenceDepth;  // 引用深度，用于链式追踪
	bool bIsStrongReference; // 是否为强引用
	
	FReferencerInfo(const FString& InReferencerName, const FString& InReferencerClass, 
					const FString& InPropertyName, UObject* InReferencerObject = nullptr, 
					int32 InDepth = 0, bool bInIsStrongReference = true)
		: ReferencerName(InReferencerName)
		, ReferencerClass(InReferencerClass)
		, PropertyName(InPropertyName)
		, ReferencerObject(InReferencerObject)
		, ReferenceDepth(InDepth)
		, bIsStrongReference(bInIsStrongReference)
	{
	}
};

/** 引用链节点 */
struct FReferenceChainNode
{
	UObject* Object;
	FString ObjectName;
	FString ClassName;
	FString PropertyName;  // 引用此对象的属性名
	int32 Depth;
	bool bIsGCRoot;
	TArray<TSharedPtr<FReferenceChainNode>> Children;
	TWeakPtr<FReferenceChainNode> Parent;
	
	FReferenceChainNode(UObject* InObject, int32 InDepth = 0)
		: Object(InObject)
		, Depth(InDepth)
		, bIsGCRoot(false)
	{
		if (IsValid(InObject))
		{
			ObjectName = InObject->GetName();
			ClassName = InObject->GetClass()->GetName();
		}
		else
		{
			ObjectName = TEXT("GC Root");
			ClassName = TEXT("Root");
			bIsGCRoot = true;
		}
	}
};

/** 搜索过滤选项 */
struct FSearchFilterOptions
{
	bool bIncludeEditorWorld = true;
	bool bIncludePIEWorld = true;
	bool bIncludeGameWorld = false;
	bool bShowMemoryInfo = true;
	bool bShowReferenceChain = true;
	bool bShowStatistics = true;
	bool bAutoRefresh = false;
	int32 MaxReferenceDepth = 5;
	
	FSearchFilterOptions() {}
};

/** 搜索统计信息 */
struct FSearchStatistics
{
	int32 TotalInstancesFound = 0;
	int32 TotalReferencersFound = 0;
	int32 MaxReferenceDepth = 0;
	int32 ObjectsWithoutReferencers = 0;
	int32 GCRootObjects = 0;
	float SearchDuration = 0.0f;
	float TotalMemoryUsage = 0.0f;
	FString MostCommonReferencer;
	TMap<FString, int32> ClassDistribution;
	TMap<FString, int32> WorldDistribution;
	
	FSearchStatistics()
	{
		Reset();
	}
	
	void Reset()
	{
		TotalInstancesFound = 0;
		TotalReferencersFound = 0;
		MaxReferenceDepth = 0;
		ObjectsWithoutReferencers = 0;
		GCRootObjects = 0;
		SearchDuration = 0.0f;
		TotalMemoryUsage = 0.0f;
		MostCommonReferencer.Empty();
		ClassDistribution.Empty();
		WorldDistribution.Empty();
	}
};

/** 搜索历史项 */
struct FSearchHistoryItem
{
	FString ClassName;
	FDateTime SearchTime;
	int32 ResultCount;
	float SearchDuration;
	
	FSearchHistoryItem(const FString& InClassName, int32 InResultCount, float InSearchDuration)
		: ClassName(InClassName)
		, SearchTime(FDateTime::Now())
		, ResultCount(InResultCount)
		, SearchDuration(InSearchDuration)
	{}
};

/** UI主题颜色 */
struct FDebuggerTheme
{
	FLinearColor GCRootColor = FLinearColor::Green;
	FLinearColor DirectReferenceColor = FLinearColor::Yellow;
	FLinearColor IndirectReferenceColor = FLinearColor::Gray;
	FLinearColor StrongReferenceColor = FLinearColor::Red;
	FLinearColor WeakReferenceColor = FLinearColor::Blue;
	FLinearColor PrimaryTextColor = FLinearColor::White;
	FLinearColor SecondaryTextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
	FLinearColor BackgroundColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
	FLinearColor BorderColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);
	
	static FDebuggerTheme& Get()
	{
		static FDebuggerTheme Instance;
		return Instance;
	}
}; 