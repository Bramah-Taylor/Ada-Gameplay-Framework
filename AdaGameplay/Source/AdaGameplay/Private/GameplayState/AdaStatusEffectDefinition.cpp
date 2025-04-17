// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaStatusEffectDefinition.h"

#include "GameplayState/AdaAttributeFunctionLibrary.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "AdaStatusEffect"

EDataValidationResult UAdaStatusEffectDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	
	if (!EffectTag.IsValid())
	{
		Context.AddError(INVTEXT("Invalid status effect tag."));
		Result = EDataValidationResult::Invalid;
	}

	for (const auto& [AttributeTag, ModifierSpec] : Modifiers)
	{
		if (!AttributeTag.IsValid())
		{
			Context.AddError(INVTEXT("Invalid attribute tag for modifier element."));
			Result = EDataValidationResult::Invalid;
		}

		if (!UAdaAttributeFunctionLibrary::IsModifierValid(ModifierSpec))
		{
			Context.AddError(FText::Format(LOCTEXT("AttributeTag", "Invalid modifier setup for modifier to attribute {0}."), FText::FromString(AttributeTag.ToString())));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE