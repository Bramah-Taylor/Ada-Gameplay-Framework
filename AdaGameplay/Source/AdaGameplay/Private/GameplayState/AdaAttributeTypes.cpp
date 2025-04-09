// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaAttributeTypes.h"

#include "Debug/AdaAssertionMacros.h"
#include "GameplayState/AdaGameplayStateComponent.h"

FAdaAttribute::FAdaAttribute(const FGameplayTag Tag, const FAdaAttributeInitParams& InitParams) :
	AttributeTag(Tag),
	ResetValue(InitParams.InitialValue),
	BaseValue(InitParams.InitialValue),
	CurrentValue(InitParams.InitialValue),
	BaseClampingValues(InitParams.InitialClampingValues),
	CurrentClampingValues(InitParams.InitialClampingValues),
	bUsesClamping(InitParams.bUsesClamping)
{
	
}

FAdaAttributePtr::FAdaAttributePtr(const TSharedRef<FAdaAttribute>& InAttribute, const UAdaGameplayStateComponent* const OwningStateComponent)
{
	AttributeTag = InAttribute.Get().AttributeTag;
	AttributeWeak = InAttribute;
	OwningStateComponentWeak = OwningStateComponent;
}

bool FAdaAttributePtr::IsValid() const
{
	return AttributeWeak.IsValid() && AttributeTag.IsValid();
}

bool FAdaAttributePtr::TryGetFromOwner()
{
	if (!OwningStateComponentWeak.IsValid() || !AttributeTag.IsValid())
	{
		return false;
	}

	const UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_ENSURE_RET(::IsValid(OwningStateComponent), false);

	FAdaAttributePtr FoundAttribute = OwningStateComponent->FindAttribute(AttributeTag);
	if (FoundAttribute.IsValid())
	{
		AttributeWeak = FoundAttribute.AttributeWeak;
		return true;
	}

	return false;
}

const FAdaAttribute* FAdaAttributePtr::Get() const
{
	const TSharedPtr<FAdaAttribute> AttributePtr = AttributeWeak.Pin();
	if (!AttributePtr.IsValid())
	{
		return nullptr;
	}
	
	return AttributePtr.Get();
}

void FAdaAttributePtr::Invalidate()
{
	AttributeWeak = nullptr;
	AttributeTag = FGameplayTag::EmptyTag;
}