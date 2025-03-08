// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaAttributeModifierTypes.h"

#include "GameplayState/AdaGameplayStateComponent.h"
#include "Debug/AdaAssertionMacros.h"

void FAdaAttributeModifierSpec::SetPeriodicData(uint8 InInterval, uint32 InDuration, bool bApplyOnAdd, bool bApplyOnRemoval)
{
	if (ApplicationType != EAdaAttributeModApplicationType::Periodic)
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Attempted to set periodic data on a non-periodic modifier."), __FUNCTION__);
		return;
	}
	
	Interval = InInterval;
	Duration = InDuration;
	bRecalculateImmediately = bApplyOnAdd;
	bShouldApplyOnRemoval = bApplyOnRemoval;
}

void FAdaAttributeModifierSpec::SetDurationData(uint32 InDuration)
{
	Duration = InDuration;
	bRecalculateImmediately = true;
}

FAdaAttributeModifier::FAdaAttributeModifier(const FGameplayTag Attribute, const FAdaAttributeModifierSpec& ModifierSpec, const uint64& CurrentFrame)
{
	AffectedAttribute = Attribute;
	
	ApplicationType = ModifierSpec.ApplicationType;
	CalculationType = ModifierSpec.CalculationType;
	OperationType = ModifierSpec.OperationType;

	bAffectsBase = ModifierSpec.bAffectsBase;
	ModifierValue = ModifierSpec.ModifierValue;
	
	Interval = ModifierSpec.Interval;
	Duration = ModifierSpec.Duration;

	bShouldApplyOnRemoval = ModifierSpec.bShouldApplyOnRemoval;
	bShouldApplyOnAdd = ModifierSpec.bRecalculateImmediately;

	if (ApplicationType == EAdaAttributeModApplicationType::Periodic)
	{
		LastApplicationFrame = CurrentFrame;
		StartFrame = CurrentFrame;
	}
	else if (ApplicationType == EAdaAttributeModApplicationType::Duration || ApplicationType == EAdaAttributeModApplicationType::Ticking)
	{
		StartFrame = CurrentFrame;
	}
}

bool FAdaAttributeModifier::HasDuration() const
{
	return ApplicationType == EAdaAttributeModApplicationType::Periodic
	|| ApplicationType == EAdaAttributeModApplicationType::Duration
	|| (ApplicationType == EAdaAttributeModApplicationType::Ticking && Duration != 0);
}

bool FAdaAttributeModifier::HasExpired(const uint64& CurrentFrame) const
{
	if (!HasDuration())
	{
		return false;
	}

	return CurrentFrame >= StartFrame + Duration;
}

bool FAdaAttributeModifier::ShouldRecalculate()
{
	if (CalculationType == EAdaAttributeModCalcType::SetByCaller
		|| CalculationType == EAdaAttributeModCalcType::SetExternally
		|| CalculationType == EAdaAttributeModCalcType::SetByAttribute)
	{
		return false;
	}
	
	// #TODO: Implement.
	return false;
}

bool FAdaAttributeModifier::CanApply(const uint64& CurrentFrame)
{
	switch (ApplicationType)
	{
		case EAdaAttributeModApplicationType::Periodic:
		{
			if (bShouldApplyOnAdd && !bHasAppliedOnAdd)
			{
				bHasAppliedOnAdd = true;
				return true;
			}
			
			return CurrentFrame - LastApplicationFrame >= Interval;
		}
		case EAdaAttributeModApplicationType::Persistent:
		{
			[[fallthrough]];
		}
		case EAdaAttributeModApplicationType::Ticking:
		{
			[[fallthrough]];
		}
		case EAdaAttributeModApplicationType::Duration:
		{
			return true;
		}
		default: break;
	}

	return false;
}

float FAdaAttributeModifier::CalculateValue()
{
	switch (CalculationType)
	{
		case EAdaAttributeModCalcType::SetByCaller:
		{
			// Doesn't change.
			[[fallthrough]];
		}
		case EAdaAttributeModCalcType::SetByAttribute:
		{
			// Updated when the modifying attribute changes.
			[[fallthrough]];
		}
		case EAdaAttributeModCalcType::SetExternally:
		{
			// Updated externally.
			return ModifierValue;
		}
		case EAdaAttributeModCalcType::SetByDelegate:
		{
			// #TODO: Implement.
			return ModifierValue;
		}
		case EAdaAttributeModCalcType::SetByEffect:
        {
        	// #TODO: Implement.
        	return ModifierValue;
        }
		case EAdaAttributeModCalcType::SetByData:
		{
			// #TODO: Implement.
			return ModifierValue;
		}
	}

	return ModifierValue;
}

void FAdaAttributeModifier::SetValue(float NewValue)
{
	ModifierValue = NewValue;
}

void FAdaAttributeModifier::PostApply(const uint64& CurrentFrame)
{
	if (ApplicationType == EAdaAttributeModApplicationType::Periodic)
	{
		LastApplicationFrame = CurrentFrame;
	}
}

void FAdaAttributeModifier::SetModifyingAttribute(const FAdaAttribute& InAttribute)
{
	A_ENSURE_RET(CalculationType == EAdaAttributeModCalcType::SetByAttribute, void(0));

	ModifyingAttribute = InAttribute.AttributeTag;
	ModifierValue = InAttribute.GetCurrentValue();
}

FAdaAttributeModifierHandle::FAdaAttributeModifierHandle(UAdaGameplayStateComponent* Owner, EAdaAttributeModApplicationType Type, int32 NewIndex)
{
	OwningStateComponentWeak = Owner;
	ApplicationType = Type;
	Index = NewIndex;
}

bool FAdaAttributeModifierHandle::IsValid() const
{
	UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, false);

	if (OwningStateComponent->FindModifier(Index))
	{
		return true;
	}

	return false;
}

void FAdaAttributeModifierHandle::Invalidate()
{
	OwningStateComponentWeak = nullptr;

	ApplicationType = EAdaAttributeModApplicationType::Instant;
	Index = -1;
}

FAdaAttributeModifier* FAdaAttributeModifierHandle::Get()
{
	UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, nullptr);

	TOptional<TSharedRef<FAdaAttributeModifier>> ModifierOptional = OwningStateComponent->FindModifier(Index);
	if (!ModifierOptional.IsSet())
	{
		return nullptr;
	}

	return &*ModifierOptional.GetValue();
}

const FAdaAttributeModifier* FAdaAttributeModifierHandle::Get() const
{
	const UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, nullptr);

	const TOptional<TSharedRef<FAdaAttributeModifier>> ModifierOptional = OwningStateComponent->FindModifier(Index);
	if (!ModifierOptional.IsSet())
	{
		return nullptr;
	}

	return &*ModifierOptional.GetValue();
}

bool FAdaAttributeModifierHandle::Remove()
{
	UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, false);

	return OwningStateComponent->RemoveModifier(*this);
}