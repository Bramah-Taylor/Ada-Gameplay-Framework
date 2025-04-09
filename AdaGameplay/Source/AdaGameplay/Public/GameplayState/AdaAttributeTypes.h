// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "AdaAttributeTypes.generated.h"

struct FAdaAttributeModifier;
class UAdaGameplayStateComponent;

DECLARE_MULTICAST_DELEGATE_FiveParams(FAdaOnAttributeUpdated, const FGameplayTag /*AttributeTag*/, const float /*NewBase*/, const float /*NewCurrent*/, const float /*OldBase*/, const float /*OldCurrent*/);

// Struct exposing parameters for initializing an attribute.
// This limits exposure to the actual live data used by the struct and hands full control of initialization over to the attribute system.
USTRUCT(BlueprintType)
struct ADAGAMEPLAY_API FAdaAttributeInitParams
{
	GENERATED_BODY()

	// The initial value this attribute should have.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float InitialValue = 0.0f;

	// Whether this attribute uses clamping or not.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bUsesClamping = false;

	// The initial values we should apply to clamp this attribute, if we're using clamping.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "bUsesClamping", EditConditionHides))
	FVector2D InitialClampingValues = FVector2D::ZeroVector;
};

// An attribute can be any arbitrary gameplay value.
// Common examples include health, stamina, and max move speed, but this can be extended to be practically any value seen in a wide array of games.
// Attributes themselves are simple data storage defining a set of values and what's currently affecting them.
// We break this down into a base (stable) and current (live) value. A more comprehensive explanation can be found in AdaGameplayStateComponent.h
USTRUCT(BlueprintType)
struct ADAGAMEPLAY_API FAdaAttribute
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;
	
public:
	FAdaAttribute() = default;
	FAdaAttribute(const FGameplayTag Tag, const FAdaAttributeInitParams& InitParams);
	
	FORCEINLINE float GetBaseValue() const { return BaseValue; };
	FORCEINLINE float GetCurrentValue() const { return CurrentValue; };
	FORCEINLINE float GetMaxValue(const bool bUseBase = false) const { return bUseBase ? BaseClampingValues.Y : CurrentClampingValues.Y; };
	FORCEINLINE float GetMinValue(const bool bUseBase = false) const { return bUseBase ? BaseClampingValues.X : CurrentClampingValues.X; };
	FORCEINLINE float GetModifierCount() const { return ActiveModifiers.Num(); };
	FORCEINLINE float GetDependencyCount() const { return AttributeDependencies.Num(); };

public:
	// The gameplay tag representing this attribute.
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AttributeTag = FGameplayTag::EmptyTag;

	// Multicast delegate that is triggered whenever this attribute's value changes.
	FAdaOnAttributeUpdated OnAttributeUpdated;
	
protected:
	// An optional fallback value to revert this attribute to if desired.
	UPROPERTY(BlueprintReadOnly)
	float ResetValue = 0.0f;
	
	// The persistent value of this attribute.
	UPROPERTY(BlueprintReadOnly)
	float BaseValue = 0.0f;

	// The value of this attribute after temporary modifiers have been calculated and applied.
	UPROPERTY(BlueprintReadOnly)
	float CurrentValue = 0.0f;

	// Clamping values applied to the base value of this attribute.
	UPROPERTY(BlueprintReadOnly)
	FVector2D BaseClampingValues = FVector2D::ZeroVector;

	// Clamping values applied to the current value of this attribute.
	UPROPERTY(BlueprintReadOnly)
	FVector2D CurrentClampingValues = FVector2D::ZeroVector;

	// Whether this attribute uses clamping or not.
	UPROPERTY(BlueprintReadOnly)
	bool bUsesClamping = false;

private:
	// An array of weak pointers to modifiers that are currently being applied to this attribute.
	TArray<TWeakPtr<FAdaAttributeModifier>> ActiveModifiers;

	// Weak pointer to an overriding modifier if one is currently being applied to this attribute.
	TWeakPtr<FAdaAttributeModifier> OverridingModifier = nullptr;

	// Map of dependencies to attribute indices on the owning gameplay state component.
	TMap<FGameplayTag, int32> AttributeDependencies;

	// Whether this attribute is currently pending recalculation.
	bool bIsDirty = false;

	// Whether this attribute is currently being overridden by an override modifier or not.
	bool bIsOverridden = false;
};

// Wrapper for a pointer to an attribute.
// Provides additional functionality for fixing up an invalid state in situations where we've lost the weak pointer
// but still want to maintain the wrapper for whatever reason.
USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributePtr
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	FAdaAttributePtr() = default;
	FAdaAttributePtr(const TSharedRef<FAdaAttribute>& InAttribute, const UAdaGameplayStateComponent* const OwningStateComponent);
	
	bool IsValid() const;
	bool TryGetFromOwner();

	const FAdaAttribute* Get() const;

public:
	FGameplayTag AttributeTag = FGameplayTag::EmptyTag;

private:
	void Invalidate();

private:
	TWeakPtr<FAdaAttribute> AttributeWeak = nullptr;
	TWeakObjectPtr<const UAdaGameplayStateComponent> OwningStateComponentWeak = nullptr;
};