// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "Delegates/AdaDelegateStack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AdaDelegateStack)

DEFINE_LOG_CATEGORY(LogAdaDelegateStack);

bool FAdaStackDelegateHandle::IsValid() const
{
	return HandleID != INDEX_NONE;
}

void FAdaStackDelegateHandle::Invalidate()
{
	HandleID = INDEX_NONE;
}

void FAdaDelegateStack::UnregisterDelegate(FAdaStackDelegateHandle& DelegateHandle)
{
	// Previously invalidated, ignore.
	if (DelegateHandle.HandleID == INDEX_NONE)
	{
		return;
	}
	
	int32 FoundIndex = INDEX_NONE;
	for (int32 i = 0; i < CallbackStack.Num(); i++)
	{
		if (CallbackStack[i].HandleID == DelegateHandle.HandleID)
		{
			FoundIndex = i;
			break;
		}
	}
	
	if (FoundIndex < 0)
	{
		UE_LOG(LogAdaDelegateStack, Error, TEXT("%hs: Attempting to unregister invalid handle."), __FUNCTION__);
	}
	else
	{
		CallbackStack.RemoveAt(FoundIndex);
	}
}

FAdaStackDelegateHandle FAdaDelegateStack::Push(const FAdaDelegateStackParams& StackParams, TFunction<void()>&& Callback)
{
	FCallbackData EntryData = FCallbackData();
	EntryData.Callback = Callback;
	EntryData.StackBehavior = StackParams.StackBehavior;
	EntryData.HandleID = ++LatestHandleID;
	
	FAdaStackDelegateHandle Handle;
	Handle.HandleID = EntryData.HandleID;
	
	CallbackStack.Push(MoveTemp(EntryData));
	
	return Handle;
}

void FAdaDelegateStack::Pop()
{
	const FCallbackData EntryData = CallbackStack.Pop();
	EntryData.Callback();
	
	if (EntryData.StackBehavior == EAdaDelegateCallbackType::CallNext)
	{
		Pop();
	}
}