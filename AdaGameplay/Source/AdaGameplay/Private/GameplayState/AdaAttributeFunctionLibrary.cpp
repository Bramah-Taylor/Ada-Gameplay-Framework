// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaAttributeFunctionLibrary.h"

#include "GameplayState/AdaAttributeTypes.h"
#include "GameplayState/AdaGameplayStateComponent.h"

FAdaAttributeModifierHandle UAdaAttributeFunctionLibrary::InhibitAttribute(UAdaGameplayStateComponent& StateComponent, const FGameplayTag AttributeTag)
{
	TSharedPtr<FAdaAttribute> FoundAttribute = StateComponent.FindAttribute_Internal(AttributeTag);
	if (!FoundAttribute.IsValid())
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Invalid attribute %s"), __FUNCTION__, *AttributeTag.ToString());
		return FAdaAttributeModifierHandle();
	}

	FAdaAttributeModifierSpec Modifier;
	Modifier.ApplicationType = EAdaAttributeModApplicationType::Persistent;
	Modifier.CalculationType = EAdaAttributeModCalcType::SetByCaller;
	Modifier.OperationType = EAdaAttributeModOpType::Override;
	Modifier.ModifierValue = FoundAttribute->GetCurrentValue();

	return StateComponent.ModifyAttribute(AttributeTag, Modifier);
}

bool UAdaAttributeFunctionLibrary::IsModifierValid(const FAdaAttributeModifierSpec& Modifier)
{
	bool bValidConfig = false;
	switch (Modifier.ApplicationType)
	{
		case EAdaAttributeModApplicationType::Instant:
		{
			bValidConfig = Modifier.AffectsBaseValue() && Modifier.OperationType != EAdaAttributeModOpType::Multiply;
			break;
		}
		case EAdaAttributeModApplicationType::Duration:
		{
			bValidConfig = !Modifier.AffectsBaseValue();
			break;
		}
		case EAdaAttributeModApplicationType::Periodic:
		{
			bValidConfig = Modifier.AffectsBaseValue() && Modifier.OperationType != EAdaAttributeModOpType::Multiply && Modifier.OperationType != EAdaAttributeModOpType::Override;
			break;
		}
		case EAdaAttributeModApplicationType::Ticking:
		{
			bValidConfig = Modifier.AffectsBaseValue() && Modifier.OperationType != EAdaAttributeModOpType::Multiply && Modifier.OperationType != EAdaAttributeModOpType::Override;
			break;
		}
		case EAdaAttributeModApplicationType::Persistent:
		{
			bValidConfig = !Modifier.AffectsBaseValue();
			break;
		}
		default: break;
	}

	bValidConfig &= IsModifierClampingValid(Modifier);

	return bValidConfig;
}

bool UAdaAttributeFunctionLibrary::IsModifierClampingValid(const FAdaAttributeModifierSpec& Modifier)
{
	switch (Modifier.ApplicationType)
	{
		case EAdaAttributeModApplicationType::Instant:
		{
			[[fallthrough]];
		}
		case EAdaAttributeModApplicationType::Duration:
		{
			return true;
		}
		case EAdaAttributeModApplicationType::Periodic:
		{
			[[fallthrough]];
		}
		case EAdaAttributeModApplicationType::Ticking:
		{
			return !Modifier.ModifiesClamping();
		}
		case EAdaAttributeModApplicationType::Persistent:
		{
			return true;
		}
		default: break;
	}

	return false;
}