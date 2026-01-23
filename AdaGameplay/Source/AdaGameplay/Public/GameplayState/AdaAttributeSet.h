// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "AdaAttributeTypes.h"
#include "Data/AdaTaggedTableRow.h"

#include "AdaAttributeSet.generated.h"

// Container for a set of attribute initialization data specified at design time and stored in a registry.
// Might also expand this in the future to have utility during play by e.g. becoming a container for attribute handles. Will depend on what the
// current workflow feels like.
USTRUCT(BlueprintType)
struct FAdaAttributeSet : public FAdaTaggedTableRow
{
	GENERATED_BODY()

public:
	// Begin FAdaTaggedTableRow overrides.
	virtual FGameplayTag GetRowTag() const override { return SetTag; };
	// End FAdaTaggedTableRow overrides.

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (Categories = "AttributeSet"))
	FGameplayTag SetTag = FGameplayTag::EmptyTag;
	
	// List of initialization parameters for attributes included in this attribute set.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FAdaAttributeInitParams> Attributes;
};