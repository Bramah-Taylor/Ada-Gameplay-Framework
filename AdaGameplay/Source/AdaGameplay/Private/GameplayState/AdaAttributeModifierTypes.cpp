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

void FAdaAttributeModifierSpec::SetClampingParams(TOptional<float> MinDelta, TOptional<float> MaxDelta)
{
	if (MinDelta.IsSet())
	{
		ClampingParams.bActive = true;
		ClampingParams.bHasMinDelta = true;
		ClampingParams.MinDelta = MinDelta.GetValue();
	}
	if (MaxDelta.IsSet())
	{
		ClampingParams.bActive = true;
		ClampingParams.bHasMaxDelta = true;
		ClampingParams.MaxDelta = MaxDelta.GetValue();
	}
}

bool FAdaAttributeModifierSpec::ModifiesClamping() const
{
	return ClampingParams.bActive;
}

bool FAdaAttributeModifierSpec::AffectsBaseValue() const
{
	return ApplicationType != EAdaAttributeModApplicationType::Duration && ApplicationType != EAdaAttributeModApplicationType::Persistent;
}

FAdaAttributeModifier::FAdaAttributeModifier(const FGameplayTag Attribute, const FAdaAttributeModifierSpec& ModifierSpec, const uint64& CurrentFrame, const int32 NewId) :
	AffectedAttribute(Attribute),
	ApplicationType(ModifierSpec.ApplicationType),
	CalculationType(ModifierSpec.CalculationType),
	OperationType(ModifierSpec.OperationType),
	bAffectsBase(ModifierSpec.AffectsBaseValue()),
	ModifierValue(ModifierSpec.ModifierValue),
	ClampingParams(ModifierSpec.ClampingParams),
	Interval(ModifierSpec.Interval),
	Duration(ModifierSpec.Duration),
	Identifier(NewId),
	bShouldApplyOnAdd(ModifierSpec.bRecalculateImmediately),
	bShouldApplyOnRemoval(ModifierSpec.bShouldApplyOnRemoval)
{

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

bool FAdaAttributeModifier::ModifiesClamping() const
{
	return ClampingParams.bActive;
}

bool FAdaAttributeModifier::ShouldRecalculate() const
{
	switch (CalculationType)
	{
		case EAdaAttributeModCalcType::SetByCaller:
		{
			[[fallthrough]];
		}
		case EAdaAttributeModCalcType::SetExternally:
		{
			[[fallthrough]];
		}
		case EAdaAttributeModCalcType::SetByAttribute:
		{
			return false;
		}
		case EAdaAttributeModCalcType::SetByEffect:
		{
			// #TODO: Will eventually query the effect here
			return false;
		}
		case EAdaAttributeModCalcType::SetByDelegate:
		{
			// #TODO: SetByDelegate requires a delegate to query here - we'll implement two separate delegates for querying and calculating.
		}
		case EAdaAttributeModCalcType::SetByData:
		{
			// Always re-evaluate curve-based modifiers.
			return true;
		}
		default: break;
	}
	
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
			// #TODO(Ada.Gameplay): Implement.
			return ModifierValue;
		}
		case EAdaAttributeModCalcType::SetByEffect:
        {
        	// #TODO(Ada.Gameplay): Implement.
        	return ModifierValue;
        }
		case EAdaAttributeModCalcType::SetByData:
		{
			// #TODO(Ada.Gameplay): Implement.
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

FString FAdaAttributeModifier::ToString() const
{
	FString OutString;
	OutString += TEXT("Application Type: ") + StaticEnum<EAdaAttributeModApplicationType>()->GetNameStringByValue((int64)ApplicationType);
	OutString += TEXT("\nCalculation Type: ") + StaticEnum<EAdaAttributeModCalcType>()->GetNameStringByValue((int64)CalculationType);
	OutString += TEXT("\nOperation Type: ") + StaticEnum<EAdaAttributeModOpType>()->GetNameStringByValue((int64)OperationType);

	return OutString;
}

void FAdaAttributeModifier::SetModifyingAttribute(const FAdaAttribute& InAttribute)
{
	A_ENSURE_RET(CalculationType == EAdaAttributeModCalcType::SetByAttribute, void(0));

	ModifyingAttribute = InAttribute.AttributeTag;
	ModifierValue = InAttribute.GetCurrentValue();
}

FAdaAttributeModifierHandle::FAdaAttributeModifierHandle(UAdaGameplayStateComponent* Owner, const EAdaAttributeModApplicationType Type, const int32 NewIndex, const int32 NewId) :
	OwningStateComponentWeak(Owner),
	Index(NewIndex),
	Identifier(NewId)
{
	
}

bool FAdaAttributeModifierHandle::IsValid() const
{
	if (Identifier == INDEX_NONE)
	{
		return false;
	}
	
	UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, false);

	if (OwningStateComponent->FindModifierByIndex(Index))
	{
		return true;
	}

	return false;
}

void FAdaAttributeModifierHandle::Invalidate()
{
	OwningStateComponentWeak = nullptr;
	Index = INDEX_NONE;
	Identifier = INDEX_NONE;
}

FAdaAttributeModifier* FAdaAttributeModifierHandle::Get()
{
	UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, nullptr);

	TOptional<TSharedRef<FAdaAttributeModifier>> ModifierOptional = OwningStateComponent->FindModifierByIndex(Index);
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

	const TOptional<TSharedRef<FAdaAttributeModifier>> ModifierOptional = OwningStateComponent->FindModifierByIndex(Index);
	if (!ModifierOptional.IsSet())
	{
		return nullptr;
	}

	const TSharedRef<FAdaAttributeModifier> ModifierRef = ModifierOptional.GetValue();
	if (ModifierRef->GetIdentifier() != Identifier)
	{
		return nullptr;
	}

	return ModifierRef.ToSharedPtr().Get();
}

bool FAdaAttributeModifierHandle::Remove()
{
	UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, false);

	return OwningStateComponent->RemoveModifier(*this);
}