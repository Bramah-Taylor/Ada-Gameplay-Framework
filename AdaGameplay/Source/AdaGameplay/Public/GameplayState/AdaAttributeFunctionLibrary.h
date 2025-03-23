// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AdaAttributeModifierTypes.h"

#include "AdaAttributeFunctionLibrary.generated.h"

class UAdaGameplayStateComponent;

UCLASS()
class ADAGAMEPLAY_API UAdaAttributeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Inhibit modification of an attribute.
	static FAdaAttributeModifierHandle InhibitAttribute(UAdaGameplayStateComponent& StateComponent, const FGameplayTag AttributeTag);

	static bool IsModifierValid(const FAdaAttributeModifierSpec& Modifier);
	static bool IsModifierClampingValid(const FAdaAttributeModifierSpec& Modifier);
};
