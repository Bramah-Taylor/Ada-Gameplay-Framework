// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaStatusEffect.h"

#include "GameplayState/AdaAttributeFunctionLibrary.h"
#include "GameplayState/AdaGameplayStateComponent.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "AdaStatusEffect"

FAdaStatusEffectHandle::FAdaStatusEffectHandle(UAdaGameplayStateComponent* Owner, const int32 InIndex, const int32 InId) :
	OwningStateComponentWeak(Owner),
	Index(InIndex),
	Identifier(InId)
{
	
}

bool FAdaStatusEffectHandle::IsValid() const
{
	// #TODO(Ada.Gameplay): Implement
	return false;
}

void FAdaStatusEffectHandle::Invalidate()
{
	OwningStateComponentWeak = nullptr;
	Index = INDEX_NONE;
	Identifier = INDEX_NONE;
}

const UAdaStatusEffect* FAdaStatusEffectHandle::Get() const
{
	// #TODO(Ada.Gameplay): Implement
	return nullptr;
}

bool FAdaStatusEffectHandle::Remove()
{
	// #TODO(Ada.Gameplay): Implement
	return false;
}

EDataValidationResult UAdaStatusEffect::IsDataValid(FDataValidationContext& Context) const
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