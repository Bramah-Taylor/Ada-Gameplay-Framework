// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaAttributeModifierTypes.h"

#include "GameplayState/AdaGameplayStateComponent.h"
#include "GameplayState/AdaStatusEffect.h"
#include "Debug/AdaAssertionMacros.h"
#include "GameplayState/AdaAttributeFunctionLibrary.h"

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

void FAdaAttributeModifierSpec::SetDelegate(const FAdaAttributeModifierDelegate& Delegate)
{
	ModifierDelegate = Delegate;
}

void FAdaAttributeModifierSpec::SetCurveData(const FGameplayTag CurveTag, const float InCurveSpeed, const float InCurveMultiplier)
{
	ModifierCurveTag = CurveTag;
	CurveSpeed = InCurveSpeed;
	CurveMultiplier = InCurveMultiplier;
}

void FAdaAttributeModifierSpec::SetEffectData(UAdaStatusEffect* StatusEffect)
{
	ParentStatusEffect = StatusEffect;

	if (CalculationType == EAdaAttributeModCalcType::SetByEffect)
	{
		ModifierDelegate = UAdaAttributeFunctionLibrary::MakeModifierDelegate(StatusEffect, &UAdaStatusEffect::ShouldRecalculateModifier, &UAdaStatusEffect::RecalculateModifier);
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

FAdaAttributeModifier::FAdaAttributeModifier(const FGameplayTag Attribute, const FAdaAttributeModifierSpec& ModifierSpec, const uint64& CurrentTick, const int32 NewId) :
	AffectedAttribute(Attribute),
	ApplicationType(ModifierSpec.ApplicationType),
	CalculationType(ModifierSpec.CalculationType),
	OperationType(ModifierSpec.OperationType),
	bAffectsBase(ModifierSpec.AffectsBaseValue()),
	ModifierValue(ModifierSpec.ModifierValue),
	ParentStatusEffect(ModifierSpec.ParentStatusEffect),
	ModifierDelegate(ModifierSpec.ModifierDelegate),
	ClampingParams(ModifierSpec.ClampingParams),
	Interval(ModifierSpec.Interval),
	Duration(ModifierSpec.Duration),
	Identifier(NewId),
	ModifierCurveTag(ModifierSpec.ModifierCurveTag),
	CurveSpeed(ModifierSpec.CurveSpeed),
	CurveMultiplier(ModifierSpec.CurveMultiplier),
	bShouldApplyOnAdd(ModifierSpec.bRecalculateImmediately),
	bShouldApplyOnRemoval(ModifierSpec.bShouldApplyOnRemoval)
{
	if (ApplicationType == EAdaAttributeModApplicationType::Periodic)
	{
		LastApplicationTick = CurrentTick;
		StartTick = CurrentTick;
	}
	else if (ApplicationType == EAdaAttributeModApplicationType::Duration || ApplicationType == EAdaAttributeModApplicationType::Ticking)
	{
		StartTick = CurrentTick;
	}
}

bool FAdaAttributeModifier::HasDuration() const
{
	return ApplicationType == EAdaAttributeModApplicationType::Periodic
	|| ApplicationType == EAdaAttributeModApplicationType::Duration
	|| (ApplicationType == EAdaAttributeModApplicationType::Ticking && Duration != 0);
}

bool FAdaAttributeModifier::HasExpired(const uint64& CurrentTick) const
{
	if (!HasDuration())
	{
		return false;
	}

	return CurrentTick >= StartTick + Duration;
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
			[[fallthrough]];
		}
		case EAdaAttributeModCalcType::SetByDelegate:
		{
			A_ENSURE_RET(ModifierDelegate.bIsSet, false);
			return ModifierDelegate.ShouldRecalculateModifierFunc(AffectedAttribute);
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

bool FAdaAttributeModifier::CanApply(const uint64& CurrentTick)
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
			
			return CurrentTick - LastApplicationTick >= Interval;
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
		case EAdaAttributeModCalcType::SetByEffect:
		{
			[[fallthrough]];
		}
		case EAdaAttributeModCalcType::SetByDelegate:
		{
			A_ENSURE_RET(ModifierDelegate.bIsSet, ModifierValue);
			ModifierValue = ModifierDelegate.RecalculateModifierFunc(AffectedAttribute);
			return ModifierValue;
		}
		case EAdaAttributeModCalcType::SetByData:
		{
			A_ENSURE_MSG_RET(ModifierCurve.IsValid(), ModifierValue, TEXT("%hs: Invalid Curve Float asset for modifier %s."), __FUNCTION__, *ModifierCurveTag.ToString());
			ModifierValue = ModifierCurve->GetFloatValue(CurveProgress) * CurveMultiplier;
			return ModifierValue;
		}
	}

	return ModifierValue;
}

void FAdaAttributeModifier::SetValue(float NewValue)
{
	ModifierValue = NewValue;
}

void FAdaAttributeModifier::PostApply(const uint64& CurrentTick)
{
	if (ApplicationType == EAdaAttributeModApplicationType::Periodic)
	{
		LastApplicationTick = CurrentTick;
	}

	if (CalculationType == EAdaAttributeModCalcType::SetByData)
	{
		CurveProgress += CurveSpeed;
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

void FAdaAttributeModifier::SetModifierCurve(const UCurveFloat* const InModifierCurve)
{
	A_ENSURE_RET(IsValid(InModifierCurve), void(0));

	ModifierCurve = InModifierCurve;
}

FAdaAttributeModifierHandle::FAdaAttributeModifierHandle(UAdaGameplayStateComponent* const Owner, const int32 NewIndex, const int32 NewId) :
	OwningStateComponentWeak(Owner),
	Index(NewIndex),
	Identifier(NewId)
{
	
}

bool FAdaAttributeModifierHandle::IsValid(bool bValidateOwner) const
{
	if (Identifier == INDEX_NONE || Index == INDEX_NONE)
	{
		return false;
	}

	if (!bValidateOwner)
	{
		return true;
	}
	
	const UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, false);

	const FAdaAttributeModifier* const Modifier = OwningStateComponent->FindModifierByIndex(Index);
	if (!Modifier)
	{
		return false;
	}

	return Modifier->GetIdentifier() == Identifier;
}

void FAdaAttributeModifierHandle::Invalidate()
{
	OwningStateComponentWeak = nullptr;
	Index = INDEX_NONE;
	Identifier = INDEX_NONE;
}

const FAdaAttributeModifier* FAdaAttributeModifierHandle::Get() const
{
	const UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, nullptr);

	return OwningStateComponent->FindModifierByIndex(Index);
}

bool FAdaAttributeModifierHandle::Remove()
{
	UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, false);

	return OwningStateComponent->RemoveModifier(*this);
}