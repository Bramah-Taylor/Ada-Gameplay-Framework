// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "AssetTypeActions/AssetTypeActions_Blueprint.h"

class FAdaAssetTypeActions_StatusEffect : public FAssetTypeActions_Base
{
public:
	// Begin FAssetTypeActions_Blueprint overrides
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	// End FAssetTypeActions_Blueprint overrides
};