// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaStatusEffectDefinition.h"

#include "GameplayState/AdaAttributeFunctionLibrary.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "AdaStatusEffect"

FPrimaryAssetId UAdaStatusEffectDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, EffectTag.GetTagName());
}

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

		TArray<FString> Errors;
		if (!UAdaAttributeFunctionLibrary::IsModifierValid(ModifierSpec, Errors, true))
		{
			Context.AddError(FText::Format(LOCTEXT("AttributeTag", "Invalid modifier setup for modifier to attribute {0}."), FText::FromString(AttributeTag.ToString())));
			for (const FString& ErrorString : Errors)
			{
				Context.AddError(FText::FromString(ErrorString));
			}
			
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE