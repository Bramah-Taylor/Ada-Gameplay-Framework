// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdaTagCountContainer, Log, All);

// Struct ported over from GAS, minus the delegate & parent tag logic.
struct ADAGAMEPLAY_API FAdaGameplayTagCountContainer
{	
	FAdaGameplayTagCountContainer()
	{}

	/// @brief	Check if the count container has a gameplay tag that matches against the specified tag (expands to include parents of asset tags)
	/// @param	TagToCheck		Tag to check for a match
	/// @return	True if the count container has a gameplay tag that matches, false if not
	inline bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const
	{
		return TagCountMap.FindRef(TagToCheck) > 0;
	}

	/// @brief	Check if the count container has gameplay tags that matches against all of the specified tags (expands to include parents of asset tags)
	/// @param TagContainer		Tag container to check for a match. If empty will return true
	/// @return	True if the count container matches all of the gameplay tags
	inline bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
	{
		// if the TagContainer count is 0 return bCountEmptyAsMatch;
		if (TagContainer.Num() == 0)
		{
			return true;
		}

		bool AllMatch = true;
		for (const FGameplayTag& Tag : TagContainer)
		{
			if (TagCountMap.FindRef(Tag) <= 0)
			{
				AllMatch = false;
				break;
			}
		}		
		return AllMatch;
	}
	
	/// @brief	Check if the count container has gameplay tags that matches against any of the specified tags (expands to include parents of asset tags)
	/// @param	TagContainer	Tag container to check for a match. If empty will return false
	/// @return True if the count container matches any of the gameplay tags
	inline bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
	{
		if (TagContainer.Num() == 0)
		{
			return false;
		}

		bool AnyMatch = false;
		for (const FGameplayTag& Tag : TagContainer)
		{
			if (TagCountMap.FindRef(Tag) > 0)
			{
				AnyMatch = true;
				break;
			}
		}
		
		return AnyMatch;
	}
	
	/// @brief	Update the specified container of tags by the specified delta, potentially causing an additional or removal from the explicit tag list
	/// @param	Container		Container of tags to update
	/// @param	CountDelta		Delta of the tag count to apply
	inline void UpdateTagCount(const FGameplayTagContainer& Container, int32 CountDelta)
	{
		if (CountDelta != 0)
		{
			bool bUpdatedAny = false;
			for (auto TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
			{
				bUpdatedAny |= UpdateTagMap_Internal(*TagIt, CountDelta);
			}
		}
	}
	
	/// @brief	Update the specified tag by the specified delta, potentially causing an additional or removal from the explicit tag list
	/// @param	Tag				Tag to update
	/// @param	CountDelta		Delta of the tag count to apply
	/// @return True if tag was *either* added or removed. (E.g., we had the tag and now don't, or didn't have the tag and now we do. We didn't just change the count (1 count -> 2 count would return false).
	inline bool UpdateTagCount(const FGameplayTag& Tag, int32 CountDelta)
	{
		if (CountDelta != 0)
		{
			return UpdateTagMap_Internal(Tag, CountDelta);
		}

		return false;
	}

	/// @brief	Set the specified tag count to a specific value
	/// @param	Tag				Tag to update
	/// @param	NewCount		New count of the tag
	/// @return	True if tag was *either* added or removed. (E.g., we had the tag and now don't, or didn't have the tag and now we do. We didn't just change the count (1 count -> 2 count would return false).
	inline bool SetTagCount(const FGameplayTag& Tag, int32 NewCount)
	{
		int32 ExistingCount = 0;
		if (int32* Ptr  = TagCountMap.Find(Tag))
		{
			ExistingCount = *Ptr;
		}

		int32 CountDelta = NewCount - ExistingCount;
		if (CountDelta != 0)
		{
			return UpdateTagMap_Internal(Tag, CountDelta);
		}

		return false;
	}

	/// @brief	Return how many times the exact specified tag has been added to the container (ignores the tag hierarchy)
	///			e.g. if A.B & A.C were added, GetExplicitTagCount("A") would return 0, and GetExplicitTagCount("A.B") would return 1.
	/// @param	Tag				Tag to update
	/// @return	The count of the passed in tag.
	inline int32 GetTagCount(const FGameplayTag& Tag) const
	{
		if (const int32* Ptr = TagCountMap.Find(Tag))
		{
			return *Ptr;
		}

		return 0;
	}

	/// @brief	Simple accessor to the explicit gameplay tag list
	inline const FGameplayTagContainer& GetTags() const { return Tags; }

	/// @brief	Removes all of the tags.
	void Reset();

private:
	/// Map of tag to explicit count of that tag. Cannot share with above map because it's not safe to merge explicit and generic counts.
	TMap<FGameplayTag, int32> TagCountMap;

	/// Container of tags that were explicitly added.
	FGameplayTagContainer Tags;

	/// Internal helper function to adjust the explicit tag list & corresponding maps/delegates/etc. as necessary.
	bool UpdateTagMap_Internal(const FGameplayTag& Tag, int32 CountDelta);

	/// Internal helper function to adjust the explicit tag list & corresponding map.
	bool UpdateTags(const FGameplayTag& Tag, int32 CountDelta, bool bDeferParentTagsOnRemove);
};