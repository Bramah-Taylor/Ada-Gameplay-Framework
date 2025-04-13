// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "AdaAttributeModifierTypes.h"

#include "AdaStatusEffect.generated.h"

UCLASS(Blueprintable, Category = "Ada Gameplay")
class ADAGAMEPLAY_API UAdaStatusEffect : public UObject
{
	GENERATED_BODY()

protected:
	// Tag identifying this status effect.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Definition")
	FGameplayTag EffectTag = FGameplayTag::EmptyTag;

	// Categories that define what this status effect is.
	// E.g. if it damages a player's health, it may belong to the Damage category.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Definition")
	FGameplayTagContainer TagCategories = FGameplayTagContainer::EmptyContainer;

	// State tags to add to the target component.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State")
	FGameplayTagContainer StateTagsToAdd = FGameplayTagContainer::EmptyContainer;

	// List of tags that will prevent this status effect from being applied to the target component.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State")
	FGameplayTagContainer BlockingTags = FGameplayTagContainer::EmptyContainer;

	// List of tags that must be present in order for this status effect to be applied to the target component.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "State")
	FGameplayTagContainer EnablingTags = FGameplayTagContainer::EmptyContainer;

	// List of tags for status effects that should be cancelled by this effect's application.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FGameplayTagContainer EffectsToCancel = FGameplayTagContainer::EmptyContainer;

	// List of tags for status effects within the given category that should be cancelled by this effect's application.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	FGameplayTagContainer EffectTypesToCancel = FGameplayTagContainer::EmptyContainer;

	// List of attribute modifiers that this status effect should apply to the target component.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Modifiers", Meta = (ForceInlineRow))
	TMap<FGameplayTag, FAdaAttributeModifierSpec> Modifiers;

	// #TODO: Add data validation.
	// #TODO: Add integer-based unique id.
};

// #TODO: Make status effect handle struct.