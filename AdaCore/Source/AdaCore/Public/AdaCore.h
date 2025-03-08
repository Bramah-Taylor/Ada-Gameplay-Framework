// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FAdaCoreModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
