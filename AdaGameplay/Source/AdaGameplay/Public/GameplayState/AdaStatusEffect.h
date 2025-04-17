// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "AdaAttributeModifierTypes.h"

#include "AdaStatusEffect.generated.h"

class FDataValidationContext;
class UAdaGameplayStateComponent;

USTRUCT()
struct ADAGAMEPLAY_API FAdaStatusEffectHandle
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	FAdaStatusEffectHandle() = default;
	FAdaStatusEffectHandle(UAdaGameplayStateComponent* Owner, const int32 InIndex, const int32 InId);
	
	bool IsValid() const;
	void Invalidate();

	const UAdaStatusEffect* Get() const;

	bool Remove();
	
private:
	// The component that owns this status effect.
	TWeakObjectPtr<UAdaGameplayStateComponent> OwningStateComponentWeak = nullptr;

	// The index in the owning component's array for this status effect, when it's valid.
	int32 Index = INDEX_NONE;

	// The identifier assigned to this status effect.
	int32 Identifier = INDEX_NONE;
};

UCLASS(Blueprintable, Category = "Ada Gameplay")
class ADAGAMEPLAY_API UAdaStatusEffect : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

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

	UPROPERTY()
	int32 EffectId = INDEX_NONE;

	// #TODO: Add data validation.
};