// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameFramework/AdaGameplayTagCountContainer.h"

DEFINE_LOG_CATEGORY(LogAdaTagCountContainer);

void FAdaGameplayTagCountContainer::Reset()
{
	TagCountMap.Reset();
	Tags.Reset();
}

bool FAdaGameplayTagCountContainer::UpdateTags(const FGameplayTag& Tag, const int32 CountDelta, const bool bDeferParentTagsOnRemove)
{
	const bool bTagAlreadyExists = Tags.HasTagExact(Tag);

	// Need special case handling to maintain the explicit tag list correctly, adding the tag to the list if it didn't previously exist and a
	// positive delta comes in, and removing it from the list if it did exist and a negative delta comes in.
	if (!bTagAlreadyExists)
	{
		// Brand new tag with a positive delta needs to be explicitly added
		if (CountDelta > 0)
		{
			Tags.AddTag(Tag);
		}
		// Block attempted reduction of non-explicit tags, as they were never truly added to the container directly
		else
		{
			// Only warn about tags that are in the container but will not be removed because they aren't explicitly in the container
			if (Tags.HasTag(Tag))
			{
				UE_LOG(LogAdaTagCountContainer, Warning, TEXT("Attempted to remove tag: %s from tag count container, but it is not explicitly in the container!"), *Tag.ToString());
			}
			return false;
		}
	}

	// Update the explicit tag count map. This has to be separate than the map below because otherwise the count of nested tags ends up wrong
	int32& ExistingCount = TagCountMap.FindOrAdd(Tag);

	ExistingCount = FMath::Max(ExistingCount + CountDelta, 0);

	// If our new count is 0, remove us from the explicit tag list
	if (ExistingCount <= 0)
	{
		// Remove from the explicit list
		Tags.RemoveTag(Tag, bDeferParentTagsOnRemove);
	}

	return true;
}

bool FAdaGameplayTagCountContainer::UpdateTagMap_Internal(const FGameplayTag& Tag, int32 CountDelta)
{
	if (!UpdateTags(Tag, CountDelta, false))
	{
		return false;
	}

	return true;
}