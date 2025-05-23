// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaAttributeFunctionLibrary.h"

#include "GameplayState/AdaAttributeTypes.h"
#include "GameplayState/AdaGameplayStateComponent.h"

FAdaAttributeModifierHandle UAdaAttributeFunctionLibrary::InhibitAttribute(UAdaGameplayStateComponent& StateComponent, const FGameplayTag AttributeTag)
{
	FAdaAttribute* FoundAttribute = StateComponent.FindAttribute_Internal(AttributeTag);
	if (!FoundAttribute)
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

bool UAdaAttributeFunctionLibrary::IsModifierValid(const FAdaAttributeModifierSpec& Modifier, TArray<FString>& OutErrors, const bool bEditorContext)
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

	if (!bValidConfig)
	{
		OutErrors.Add("Invalid Application config.");
	}

	const bool bValidClampingConfig = IsModifierClampingValid(Modifier);
	if (!bValidClampingConfig)
	{
		OutErrors.Add(("Invalid modifier clamping config."));
	}

	bool bValidDelegateConfig = false;
	if (Modifier.CalculationType == EAdaAttributeModCalcType::SetByDelegate
		|| (Modifier.CalculationType == EAdaAttributeModCalcType::SetByEffect && !bEditorContext))
	{
		bValidDelegateConfig = Modifier.ModifierDelegate.bIsSet;
	}
	else
	{
		bValidDelegateConfig = !Modifier.ModifierDelegate.bIsSet;
	}

	if (!bValidDelegateConfig)
	{
		OutErrors.Add("Invalid delegate config.");
	}

	return bValidConfig && bValidClampingConfig && bValidDelegateConfig;
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