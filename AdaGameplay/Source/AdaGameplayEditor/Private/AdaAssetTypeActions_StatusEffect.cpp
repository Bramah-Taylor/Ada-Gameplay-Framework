// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "AdaAssetTypeActions_StatusEffect.h"

#include "AdaStatusEffectFactory.h"
#include "GameplayState/AdaStatusEffectDefinition.h"

FText FAdaAssetTypeActions_StatusEffect::GetName() const
{ 
	return INVTEXT("Ada Status Effect"); 
}

FColor FAdaAssetTypeActions_StatusEffect::GetTypeColor() const
{
	return FColor(128, 0, 128);
}

UClass* FAdaAssetTypeActions_StatusEffect::GetSupportedClass() const
{
	return UAdaStatusEffectDefinition::StaticClass();
}

uint32 FAdaAssetTypeActions_StatusEffect::GetCategories()
{
	return EAssetTypeCategories::Gameplay;
}