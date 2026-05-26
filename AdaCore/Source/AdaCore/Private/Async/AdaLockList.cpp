// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "Async/AdaLockList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AdaLockList)

void FAdaLockList::AddLock(const FGameplayTag LockReason)
{
	LockReasons.AddTag(LockReason);
}

void FAdaLockList::RemoveLock(const FGameplayTag LockReason)
{
	LockReasons.RemoveTag(LockReason);
}

void FAdaLockList::ClearLocks()
{
	LockReasons.Reset();
}

bool FAdaLockList::IsLocked() const
{
	return !LockReasons.IsEmpty();
}

bool FAdaLockList::IsEmpty() const
{
	return LockReasons.IsEmpty();
}

bool FAdaLockList::HasLock(const FGameplayTag LockReason) const
{
	return LockReasons.HasTagExact(LockReason);
}