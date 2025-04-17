// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions/AssetTypeActions_Blueprint.h"

class FAssetTypeActions_StatusEffect : public FAssetTypeActions_Blueprint
{
public:
	// Begin FAssetTypeActions_Blueprint overrides
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Blueprint | EAssetTypeCategories::Gameplay; }
	// End FAssetTypeActions_Blueprint overrides
};