// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "AdaAwaitSubsystem.generated.h"

USTRUCT()
struct FAdaAwaitListenerHandle
{
	GENERATED_BODY()
	
public:
	bool IsValid() const { return ConditionTag.IsValid(); }
	
public:
	FGameplayTag ConditionTag = FGameplayTag::EmptyTag;
	int32 HandleID = 0;
};

DECLARE_LOG_CATEGORY_EXTERN(LogAdaAwaitSubsystem, Log, All);

// Generic system designed to act as a catch-all for responding to complex conditions being met.
// The goal of a system like this is to simplify and decouple complex conditional behavior where the order of conditions is not guaranteed or the
// setting up of listening to a set of conditions is otherwise overly complicated. A common use case for this is when a client is waiting for various
// responses to come down to them from the server via a mix of replication and RPCs. With the await system, conditions can be built and notified by
// multiple systems independent of one another, allowing other systems and actors to easily listen for when those conditions are met.
UCLASS()
class ADACORE_API UAdaAwaitSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/// @brief	Static getter for the await subsystem.
	/// @return	The await subsystem for the game instance associated with the world of the specified object.
	static UAdaAwaitSubsystem* Get(const UObject* WorldContextObject);
	
	/// @brief	Check if a certain condition has been met.
	/// @param	ConditionTag	The condition we want to check.
	/// @return	Whether the condition has been met.
	bool HasConditionBeenMet(const FGameplayTag ConditionTag) const;
	
	/// @brief	Check if a set of conditions have been met.
	/// @param	Conditions	The conditions we want to check.
	/// @return	Whether all the conditions have been met.
	bool HaveAllConditionsBeenMet(const FGameplayTagContainer& Conditions) const;
	
	/// @brief	Remove the listener for the specified, unmet condition.
	/// @param	ListenerHandle	The handle for the callback associated with a specific condition.
	/// @note	It's only necessary to unregister when the condition hasn't been met, for example if we're cleaning up the listening actor.
	///			The subsystem itself handles cleaning up of listeners when conditions are met.
	void UnregisterListener(FAdaAwaitListenerHandle& ListenerHandle);
	
	/// @brief	Notify listeners that the specified condition has been met.
	/// @param	ConditionTag	The condition that has been satisfied.
	void NotifyConditionMet(const FGameplayTag ConditionTag);
	
	/// @brief	Add a sub-condition that must be met before the primary condition is satisfied.
	///			This will make the primary condition be composed of sub-conditions, which the await system will check off during relevant notifications.
	///			Once all the sub-conditions of a condition are met, the primary condition will be met and the notification will be sent out.
	/// @param	ConditionTag	The primary condition we're adding a sub-condition to.
	/// @param	SubConditionTag	The sub-condition that will form part of the primary condition.
	/// @note	Multiple systems can register sub-conditions to the same primary condition independently.
	void RegisterSubCondition(const FGameplayTag ConditionTag, const FGameplayTag SubConditionTag);
	
	/// @brief	Add sub-conditions that must be met before the primary condition is satisfied.
	///			This will make the primary condition be composed of sub-conditions, which the await system will check off during relevant notifications.
	///			Once all the sub-conditions of a condition are met, the primary condition will be met and the notification will be sent out.
	/// @param	ConditionTag	The primary condition we're adding a sub-condition to.
	/// @param	SubConditions	The sub-conditions that will form part of the primary condition.
	/// @note	Multiple systems can register sub-conditions to the same primary condition independently.
	void RegisterSubConditions(const FGameplayTag ConditionTag, const FGameplayTagContainer& SubConditions);
	
	/// @brief	Register a callback to be notified when the condition is met.
	/// @param	ConditionTag	The condition we're listening for.
	/// @param	Callback		A pointer to the callback function.
	/// @return	A handle to the listener in the subsystem, which we unregister if never notified.
	/// @note	The callback will be called immediately if the condition has already been met.
	[[nodiscard]] FAdaAwaitListenerHandle RegisterListener(const FGameplayTag ConditionTag, TFunction<void()>&& Callback);
	
	/// @brief	Register a callback to be notified when the condition is met.
	/// @param	ConditionTag	The condition we're listening for.
	/// @param	Object			The UObject instance that we'll invoke the callback function on.
	/// @param	Function		A pointer to the callback function.
	/// @return	A handle to the listener in the subsystem, which we unregister if never notified.
	/// @note	The callback will be called immediately if the condition has already been met.
	template<typename TOwner = UObject>
	[[nodiscard]] FAdaAwaitListenerHandle RegisterListener(const FGameplayTag ConditionTag, TOwner* Object, void(TOwner::* Function)())
	{
		TWeakObjectPtr<TOwner> WeakObject(Object);
		return RegisterListener(ConditionTag, [WeakObject, Function]()
		{
			if (TOwner* StrongObject = WeakObject.Get())
			{
				(StrongObject->*Function)();
			}
		});
	}
	
private:
	struct FListenerData
	{
		// Callback function to call when the owning condition has been satisfied.
		TFunction<void()> Callback;

		int32 HandleID = 0;
	};
	
	// List of listeners for a specific condition.
	// Using a struct instead of type aliasing for better legibility.
	struct FListenerList
	{
		TArray<FListenerData> Listeners;
	};
	
	int32 LatestHandleID = 0;
	
	FGameplayTagContainer SatisfiedConditions;
	
	TMap<FGameplayTag, FListenerList> ListenerMap;
	
	// Map of tags to those that depend on them.
	TMap<FGameplayTag, FGameplayTagContainer> PrimaryConditionMap;
	
	// Map of sub-conditions to the tags they're dependent upon (inverse of the above).
	TMap<FGameplayTag, FGameplayTagContainer> SubConditionMap;
};