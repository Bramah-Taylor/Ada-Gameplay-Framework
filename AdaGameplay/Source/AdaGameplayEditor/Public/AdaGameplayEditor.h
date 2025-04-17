// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FAdaAssetTypeActions_StatusEffect;

class FAdaGameplayEditorModule : public IModuleInterface
{
public:

	// Begin IModuleInterface overrides
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface overrides

protected:
	TSharedPtr<FAdaAssetTypeActions_StatusEffect> StatusEffectAssetTypeAction;
};
