// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "AssetTypeActions_StatusEffect.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

FText FAssetTypeActions_StatusEffect::GetName() const
{ 
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_StatusEffect", "Status Effect Blueprint"); 
}

FColor FAssetTypeActions_StatusEffect::GetTypeColor() const
{
	return FColor(128, 0, 128);
}

#undef LOCTEXT_NAMESPACE