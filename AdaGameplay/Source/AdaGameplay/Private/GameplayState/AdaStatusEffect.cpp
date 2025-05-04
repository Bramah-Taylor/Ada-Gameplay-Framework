// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaStatusEffect.h"

bool UAdaStatusEffect::ShouldRecalculateModifier_Implementation(const FGameplayTag AttributeTag)
{
	checkNoEntry();
	return false;
}

float UAdaStatusEffect::RecalculateModifier_Implementation(const FGameplayTag AttributeTag)
{
	checkNoEntry();
	return 0.0f;
}