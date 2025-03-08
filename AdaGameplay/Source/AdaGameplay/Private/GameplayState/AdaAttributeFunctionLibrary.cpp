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
	Modifier.bAffectsBase = false;
	Modifier.ModifierValue = FoundAttribute->GetCurrentValue();

	return StateComponent.ModifyAttribute(AttributeTag, Modifier);
}