// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaAttributeTypes.h"

#include "Debug/AdaAssertionMacros.h"
#include "GameplayState/AdaGameplayStateComponent.h"

FAdaAttribute::FAdaAttribute(const FGameplayTag Tag, const FAdaAttributeInitParams& InitParams, const int32 NewId) :
	AttributeTag(Tag),
	ResetValue(InitParams.InitialValue),
	BaseValue(InitParams.InitialValue),
	CurrentValue(InitParams.InitialValue),
	BaseClampingValues(InitParams.InitialClampingValues),
	CurrentClampingValues(InitParams.InitialClampingValues),
	bUsesClamping(InitParams.bUsesClamping),
	bTreatAsInteger(InitParams.bTreatAsInteger),
	Identifier(NewId)
{
	
}

float FAdaAttribute::GetBaseValue() const
{
	return bTreatAsInteger ? FMath::FloorToFloat(BaseValue) : BaseValue;
}

float FAdaAttribute::GetCurrentValue() const
{
	return bTreatAsInteger ? FMath::FloorToFloat(CurrentValue) : CurrentValue;
}

float FAdaAttribute::GetMaxValue(const bool bUseBase) const
{
	const float MaxValue = bUseBase ? BaseClampingValues.Y : CurrentClampingValues.Y;
	return bTreatAsInteger ? FMath::FloorToFloat(MaxValue) : MaxValue;
}

float FAdaAttribute::GetMinValue(const bool bUseBase) const
{
	const float MinValue = bUseBase ? BaseClampingValues.X : CurrentClampingValues.X;
	return bTreatAsInteger ? FMath::FloorToFloat(MinValue) : MinValue;
}

FAdaAttributeHandle::FAdaAttributeHandle(const UAdaGameplayStateComponent* const Owner, const FGameplayTag NewTag, const int32 NewIndex, const int32 NewId) :
	AttributeTag(NewTag),
	OwningStateComponentWeak(Owner),
	Index(NewIndex),
	Identifier(NewId)
{
}

bool FAdaAttributeHandle::IsValid(bool bValidateOwner) const
{
	if (Identifier == INDEX_NONE || Index == INDEX_NONE || !AttributeTag.IsValid())
	{
		return false;
	}

	if (!bValidateOwner)
	{
		return true;
	}
	
	const UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, false);

	const FAdaAttribute* Attribute = OwningStateComponent->FindAttributeByIndex(Index);
	if (!Attribute)
	{
		return false;
	}

	return Attribute->GetIdentifier() == Identifier;
}

const FAdaAttribute* FAdaAttributeHandle::Get() const
{
	const UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, nullptr);

	return OwningStateComponent->FindAttributeByIndex(Index);
}

void FAdaAttributeHandle::Invalidate()
{
	AttributeTag = FGameplayTag::EmptyTag;
	OwningStateComponentWeak = nullptr;
	Index = INDEX_NONE;
	Identifier = INDEX_NONE;
}