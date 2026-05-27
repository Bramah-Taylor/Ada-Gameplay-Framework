// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "AdaDelegateStack.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdaDelegateStack, Log, All);

UENUM()
enum class EAdaDelegateCallbackType
{
	Default,
	CallNext
};

USTRUCT()
struct ADACORE_API FAdaDelegateStackParams
{
	GENERATED_BODY()
	
	EAdaDelegateCallbackType StackBehavior = EAdaDelegateCallbackType::Default;
};

USTRUCT()
struct ADACORE_API FAdaStackDelegateHandle
{
	GENERATED_BODY()
	
	friend struct FAdaDelegateStack;

protected:
	int32 HandleID = 0;
};

// Container that represents a stack of delegates that get broadcast one at a time, LIFO, for when you don't want all the callbacks in a
// delegate's invocation list to get called on a single broadcast.
USTRUCT()
struct ADACORE_API FAdaDelegateStack
{
	GENERATED_BODY()
	
public:
	/// @brief	Remove the delegate callback for the specified handle.
	/// @param	DelegateHandle	The handle for the callback associated with a specific condition.
	void UnregisterDelegate(FAdaStackDelegateHandle& DelegateHandle);
	
	/// @brief	Register a callback for when this delegate stack is next triggered.
	/// @param	StackParams		Set of params that defines how to behave when this entry is triggered.
	/// @param	Callback		A pointer to the callback function.
	/// @return	A handle to the callback in the stack.
	[[nodiscard]] FAdaStackDelegateHandle Push(const FAdaDelegateStackParams& StackParams, TFunction<void()>&& Callback);
	
	/// @brief	Register a callback for when this delegate stack is next triggered.
	/// @param	StackParams		Set of params that defines how to behave when this entry is triggered.
	/// @param	Object			The UObject instance that we'll invoke the callback function on.
	/// @param	Function		A pointer to the callback function.
	/// @return	A handle to the callback in the stack.
	template<typename TOwner = UObject>
	[[nodiscard]] FAdaStackDelegateHandle Push(const FAdaDelegateStackParams& StackParams, TOwner* Object, void(TOwner::* Function)())
	{
		TWeakObjectPtr<TOwner> WeakObject(Object);
		return Push(StackParams, [WeakObject, Function]()
		{
			if (TOwner* StrongObject = WeakObject.Get())
			{
				(StrongObject->*Function)();
			}
		});
	}
	
	/// @brief	Call the callback function at the top of the stack.
	void Pop();
	
	/// @brief	Check if there's anything currently in the stack.
	bool IsEmpty() const { return CallbackStack.IsEmpty(); };
	
private:
	struct FCallbackData
	{
		TFunction<void()> Callback;

		EAdaDelegateCallbackType StackBehavior = EAdaDelegateCallbackType::Default;
		int32 HandleID = 0;
	};
	
	TArray<FCallbackData> CallbackStack;
	
	int32 LatestHandleID = 0;
};