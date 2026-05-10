// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "Async/AdaAwaitSubsystem.h"

#include "Debug/AdaAssertionMacros.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AdaAwaitSubsystem)

DEFINE_LOG_CATEGORY(LogAdaAwaitSubsystem);

UAdaAwaitSubsystem* UAdaAwaitSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	A_ENSURE_RET(IsValid(World), nullptr);
	
	return UGameInstance::GetSubsystem<UAdaAwaitSubsystem>(World->GetGameInstance());
}

bool UAdaAwaitSubsystem::HasConditionBeenMet(const FGameplayTag ConditionTag) const
{
	return SatisfiedConditions.HasTagExact(ConditionTag);
}

bool UAdaAwaitSubsystem::HaveAllConditionsBeenMet(const FGameplayTagContainer& Conditions) const
{
	return SatisfiedConditions.HasAllExact(Conditions);
}

void UAdaAwaitSubsystem::UnregisterListener(FAdaAwaitListenerHandle& ListenerHandle)
{
	if (!ListenerHandle.IsValid())
	{
		UE_LOG(LogAdaAwaitSubsystem, Error, TEXT("%hs: Attempting to unregister invalid handle."), __FUNCTION__);
		return;
	}
	
	FListenerList* const ListenerList = ListenerMap.Find(ListenerHandle.ConditionTag);
	
	// Invalid listeners, nothing to unregister.
	if (ListenerList == nullptr)
	{
		return;
	}
	
	int32 MatchIndex = ListenerList->Listeners.IndexOfByPredicate([ID = ListenerHandle.HandleID](const FListenerData& Other) { return Other.HandleID == ID; });
	if (MatchIndex != INDEX_NONE)
	{
		ListenerList->Listeners.RemoveAtSwap(MatchIndex);
	}
	
	if (ListenerList->Listeners.IsEmpty())
	{
		ListenerMap.Remove(ListenerHandle.ConditionTag);
	}
	
	ListenerHandle.ConditionTag = FGameplayTag::EmptyTag;
}

void UAdaAwaitSubsystem::NotifyConditionMet(const FGameplayTag ConditionTag)
{
	A_ENSURE_RET(!SatisfiedConditions.HasTagExact(ConditionTag), void());
	
	SatisfiedConditions.AddTag(ConditionTag);
	
	const FListenerList* ListenerList = ListenerMap.Find(ConditionTag);
	if (ListenerList != nullptr)
	{
		for (const FListenerData& Listener : ListenerList->Listeners)
		{
			Listener.Callback();
		}
	
		ListenerMap.Remove(ConditionTag);
	}
	
	if (const FGameplayTagContainer* const TagDependencies = PrimaryConditionMap.Find(ConditionTag))
	{
		for (const FGameplayTag& DependencyTag : *TagDependencies)
		{
			FGameplayTagContainer* const DependencySubConditions = SubConditionMap.Find(DependencyTag);
			A_ENSURE_RET(DependencySubConditions != nullptr, void());
			
			DependencySubConditions->RemoveTag(ConditionTag);
			
			if (DependencySubConditions->IsEmpty())
			{
				NotifyConditionMet(DependencyTag);
			}
		}
	}
}

void UAdaAwaitSubsystem::RegisterSubCondition(const FGameplayTag ConditionTag, const FGameplayTag SubConditionTag)
{
	RegisterSubConditions(ConditionTag, SubConditionTag.GetSingleTagContainer());
}

void UAdaAwaitSubsystem::RegisterSubConditions(const FGameplayTag ConditionTag, const FGameplayTagContainer& SubConditions)
{
	if (SatisfiedConditions.HasTagExact(ConditionTag))
	{
		UE_LOG(LogAdaAwaitSubsystem, Error, TEXT("%hs: Attempting to register subconditions for condition %s, which has already succeeded."), __FUNCTION__, *ConditionTag.ToString());
		return;
	}
	
	uint8 MetConditionCount = 0;
	for (const FGameplayTag& DependencyTag : SubConditions)
	{
		if (SatisfiedConditions.HasTagExact(DependencyTag))
		{
			MetConditionCount++;
			continue;
		}
		
		if (FGameplayTagContainer* Dependencies = PrimaryConditionMap.Find(DependencyTag))
		{
			Dependencies->AddTag(ConditionTag);
		}
		else
		{
			PrimaryConditionMap.Add(DependencyTag, ConditionTag.GetSingleTagContainer());
		}
	}
	
	if (MetConditionCount == SubConditions.Num())
	{
		UE_LOG(LogAdaAwaitSubsystem, Error, TEXT("%hs: All subconditions for condition %s have already succeeded prior to registration."), __FUNCTION__, *ConditionTag.ToString());
		return;
	}
	
	if (FGameplayTagContainer* ExistingSubConditions = SubConditionMap.Find(ConditionTag))
	{
		ExistingSubConditions->AppendTags(SubConditions);
	}
	else
	{
		SubConditionMap.Add(ConditionTag, SubConditions);
	}
}

FAdaAwaitListenerHandle UAdaAwaitSubsystem::RegisterListener(const FGameplayTag ConditionTag, TFunction<void()>&& Callback)
{
	if (SatisfiedConditions.HasTagExact(ConditionTag))
	{
		Callback();
		return FAdaAwaitListenerHandle();
	}
	
	FListenerList& ListenerList = ListenerMap.FindOrAdd(ConditionTag);
	FListenerData& ListenerData = ListenerList.Listeners.AddDefaulted_GetRef();
	ListenerData.Callback = Callback;
	ListenerData.HandleID = ++LatestHandleID;
	
	FAdaAwaitListenerHandle Handle;
	Handle.HandleID = ListenerData.HandleID;
	Handle.ConditionTag = ConditionTag;
	
	return Handle;
}