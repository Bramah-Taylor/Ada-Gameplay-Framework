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

	// Create an attribute modifier delegate for a modifier which should be recalculated dynamically via delegate functions.
	template<typename T>
	static FAdaAttributeModifierDelegate MakeModifierDelegate(T* Object, bool(T::*ShouldRecalcFunc)(const FGameplayTag), float(T::*RecalcFunc)(const FGameplayTag))
	{
		static_assert(TIsDerivedFrom<T, UObject>::Value);

		FAdaAttributeModifierDelegate NewDelegate;

		TWeakObjectPtr<T> WeakObject(Object);
		TFunction<bool(const FGameplayTag)> ShouldRecalcFunctionWrapper = [WeakObject, ShouldRecalcFunc](const FGameplayTag AttributeTag) -> bool
		{
			if (T* StrongObject = WeakObject.Get())
			{
				return (StrongObject->*ShouldRecalcFunc)(AttributeTag);
			}

			return false;
		};
		
		TFunction<float(const FGameplayTag)> RecalcFunctionWrapper = [WeakObject, RecalcFunc](const FGameplayTag AttributeTag) -> float
		{
			if (T* StrongObject = WeakObject.Get())
			{
				return (StrongObject->*RecalcFunc)(AttributeTag);
			}

			return 0.0f;
		};

		NewDelegate.bIsSet = true;
		NewDelegate.ShouldRecalculateModifierFunc = MoveTemp(ShouldRecalcFunctionWrapper);
		NewDelegate.RecalculateModifierFunc = MoveTemp(RecalcFunctionWrapper);

		return NewDelegate;
	}
};
