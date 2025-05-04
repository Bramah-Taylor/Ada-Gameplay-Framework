// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "AdaStatusEffect.generated.h"

struct FAdaAttributeModifierHandle;

UCLASS(Blueprintable)
class ADAGAMEPLAY_API UAdaStatusEffect : public UObject
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	UFUNCTION(BlueprintCallable)
	FORCEINLINE int32 GetIdentifier() const { return EffectId; };

	UFUNCTION(BlueprintCallable)
	FORCEINLINE FGameplayTag GetEffectTag() const { return EffectTag; };

	UFUNCTION(BlueprintNativeEvent)
	bool ShouldRecalculateModifier(const FGameplayTag AttributeTag);

	UFUNCTION(BlueprintNativeEvent)
	float RecalculateModifier(const FGameplayTag AttributeTag);

protected:
	UPROPERTY()
	int32 EffectId = INDEX_NONE;

	UPROPERTY()
	FGameplayTag EffectTag = FGameplayTag::EmptyTag;

	UPROPERTY()
	FGameplayTagContainer TagCategories = FGameplayTagContainer::EmptyContainer;

	TArray<FAdaAttributeModifierHandle> ActiveModifierHandles;
};
