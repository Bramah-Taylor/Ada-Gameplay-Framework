// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaStatusEffect.h"

bool UAdaStatusEffect::ShouldRecalculateModifier(const FGameplayTag AttributeTag)
{
	return ShouldRecalculateModifier_BP(AttributeTag);
}

bool UAdaStatusEffect::ShouldRecalculateModifier_BP_Implementation(const FGameplayTag AttributeTag)
{
	checkNoEntry();
	return false;
}

float UAdaStatusEffect::RecalculateModifier(const FGameplayTag AttributeTag)
{
	return RecalculateModifier_BP(AttributeTag);
}

float UAdaStatusEffect::RecalculateModifier_BP_Implementation(const FGameplayTag AttributeTag)
{
	checkNoEntry();
	return 0.0f;
}
