// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "AdaLockList.generated.h"

// Generic lock system for locking a system or feature behind multiple different reasons. This allows us to, for example, enable game pausing with
// automatic reason tracking, making it easier to implement more complicated pausing behavior without coupling systems.
// Uses gameplay tags for clarity and system interoperability, but only works with exact checks under the hood.
USTRUCT()
struct ADACORE_API FAdaLockList
{
	GENERATED_BODY()
	
public:
	/// @brief	Add a lock to the lock list. Will not add a duplicate if this reason is already present.
	/// @param	LockReason	The lock we're adding.
	void AddLock(const FGameplayTag LockReason);
	
	/// @brief	Remove a lock from the lock list, assuming it's present.
	/// @param	LockReason	The lock we're removing.
	void RemoveLock(const FGameplayTag LockReason);
	
	/// @brief	Clear the lock list entirely.
	void ClearLocks();
	
	/// @brief	Check if there are any locks currently present.
	/// @return Whether the list has any locks present, and is therefore locked.
	bool IsLocked() const;
	
	/// @brief	Check if there are no locks currently present (inverse of IsLocked, provided for readability).
	/// @return Whether the list has no locks present, and is therefore unlocked.
	bool IsEmpty() const;
	
	/// @brief	Check if a specific lock reason is present in the lock list.
	/// @param	LockReason	The lock we're checking.
	/// @return Whether the specified lock reason is currently present.
	bool HasLock(const FGameplayTag LockReason) const;
	
private:
	FGameplayTagContainer LockReasons = FGameplayTagContainer::EmptyContainer;
};