// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaGameplayStateTags.h"

namespace AdaTags::ModifierCurves
{
	UE_DEFINE_GAMEPLAY_TAG(LinearIncrease, "Modifier.Curve.Linear.Increase");
	UE_DEFINE_GAMEPLAY_TAG(LinearDecrease, "Modifier.Curve.Linear.Decrease");
	UE_DEFINE_GAMEPLAY_TAG(CurvedIncrease, "Modifier.Curve.Curved.Increase");
	UE_DEFINE_GAMEPLAY_TAG(CurvedDecrease, "Modifier.Curve.Curved.Decrease");
	UE_DEFINE_GAMEPLAY_TAG(BellCurve, "Modifier.Curve.Bell.Curve");
}