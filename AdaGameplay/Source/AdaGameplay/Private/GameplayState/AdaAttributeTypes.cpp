// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaAttributeTypes.h"

FAdaAttribute::FAdaAttribute(const FGameplayTag Tag, const FAdaAttributeInitParams& InitParams) :
	AttributeTag(Tag),
	ResetValue(InitParams.InitialValue),
	BaseValue(InitParams.InitialValue),
	CurrentValue(InitParams.InitialValue),
	MaxValue(InitParams.MaxValue),
	MinValue(InitParams.MinValue),
	bUsesClamping(InitParams.bUsesClamping)
{
	
}

FAdaAttributeHandle::FAdaAttributeHandle(const TSharedRef<FAdaAttribute>& InAttribute)
{
	AttributeTag = InAttribute.Get().AttributeTag;
	AttributeWeak = InAttribute;
}

bool FAdaAttributeHandle::IsValid() const
{
	return AttributeWeak.IsValid() && AttributeTag.IsValid();
}

const FAdaAttribute* FAdaAttributeHandle::Get() const
{
	const TSharedPtr<FAdaAttribute> AttributePtr = AttributeWeak.Pin();
	if (!AttributePtr.IsValid())
	{
		return nullptr;
	}
	
	return AttributePtr.Get();
}

void FAdaAttributeHandle::Invalidate()
{
	AttributeWeak = nullptr;
	AttributeTag = FGameplayTag::EmptyTag;
}