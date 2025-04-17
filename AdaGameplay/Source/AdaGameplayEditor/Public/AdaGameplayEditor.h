// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "IAssetTypeActions.h"
#include "Modules/ModuleManager.h"

class FAdaGameplayEditorModule : public IModuleInterface
{
public:

	// Begin IModuleInterface overrides
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface overrides

protected:
	// All created asset type actions.  Cached here so that we can unregister it during shutdown.
	TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;
};
