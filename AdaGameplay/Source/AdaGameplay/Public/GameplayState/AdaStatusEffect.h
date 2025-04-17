// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "AdaStatusEffect.generated.h"

UCLASS(Blueprintable)
class ADAGAMEPLAY_API UAdaStatusEffect : public UObject
{
	GENERATED_BODY()

protected:

	UPROPERTY()
	int32 EffectId = INDEX_NONE;
};
