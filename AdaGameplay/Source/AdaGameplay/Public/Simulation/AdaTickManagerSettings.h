// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "AdaTickManagerSettings.generated.h"

UCLASS(Config = Game, DefaultConfig)
class ADAGAMEPLAY_API UAdaTickManagerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAdaTickManagerSettings() {};

	/// The default ticks per second that the system as a whole aims for.
	UPROPERTY(config, EditAnywhere, Category = "Tick Settings")
	uint8 TargetTPS = 30;

	/// Whether to use aggregated ticks or not.
	/// @note: Currently unsupported.
	UPROPERTY(config, EditAnywhere, Category = "Tick Settings")
	bool bUseAggregatedTicks = false;
};
