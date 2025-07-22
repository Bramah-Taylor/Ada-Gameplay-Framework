// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "GameplayState/AdaAttributeModifierTypes.h"

#include "AdaAttributeTypes.generated.h"

struct FAdaAttributeModifier;
class UAdaGameplayStateComponent;

DECLARE_MULTICAST_DELEGATE_FiveParams(FAdaOnAttributeUpdated, const FGameplayTag /*AttributeTag*/, const float /*NewBase*/, const float /*NewCurrent*/, const float /*OldBase*/, const float /*OldCurrent*/);
DECLARE_MULTICAST_DELEGATE_FourParams(FAdaOnClampingValueHit, const FGameplayTag /*AttributeTag*/, const float /*CurrentValue*/, const bool /*bIsMin*/, const bool /*bIsBase*/);

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

	// Whether the values of this attribute should be treated as an integer.
	// Value getters for this attribute will floor the float value before returning it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bTreatAsInteger = false;
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
	FAdaAttribute(const FGameplayTag Tag, const FAdaAttributeInitParams& InitParams, const int32 NewId);
	
	float GetBaseValue() const;
	float GetCurrentValue() const;
	float GetMaxValue(const bool bUseBase = false) const;
	float GetMinValue(const bool bUseBase = false) const;
	
	inline int32 GetModifierCount() const { return ActiveModifiers.Num(); };
	inline int32 GetDependencyCount() const { return AttributeDependencies.Num(); };
	inline int32 GetIdentifier() const { return Identifier; };

public:
	// The gameplay tag representing this attribute.
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AttributeTag = FGameplayTag::EmptyTag;

	// Multicast delegate that is triggered whenever this attribute's value changes.
	FAdaOnAttributeUpdated OnAttributeUpdated;

	// Multicast delegate that is triggered whenever one of this attribute's clamping params is reached.
	FAdaOnClampingValueHit OnClampingValueHit;
	
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

	// Whether the values of this attribute should be treated as an integer.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bTreatAsInteger = false;

	// The identifier for this attribute.
	// Used to check validity of attribute handles.
	UPROPERTY(BlueprintReadOnly)
	int32 Identifier = INDEX_NONE;

private:
	// An array of handles to modifiers that are currently being applied to this attribute.
	TArray<FAdaAttributeModifierHandle> ActiveModifiers;

	// Handle to an overriding modifier if one is currently being applied to this attribute.
	FAdaAttributeModifierHandle OverridingModifier;

	// Map of dependencies to modifier indices on the owning gameplay state component.
	TMap<FGameplayTag, int32> AttributeDependencies;

	// Whether this attribute is currently pending recalculation.
	bool bIsDirty = false;

	// Whether this attribute is currently being overridden by an override modifier or not.
	bool bIsOverridden = false;
};

// Handle to an attribute on a gameplay state component.
USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeHandle
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	FAdaAttributeHandle() = default;
	FAdaAttributeHandle(const UAdaGameplayStateComponent* const Owner, const FGameplayTag NewTag, const int32 NewIndex, const int32 NewId);
	
	bool IsValid(bool bValidateOwner = false) const;

	const FAdaAttribute* Get() const;

public:
	FGameplayTag AttributeTag = FGameplayTag::EmptyTag;

private:
	void Invalidate();

private:
	// The component that owns this attribute.
	TWeakObjectPtr<const UAdaGameplayStateComponent> OwningStateComponentWeak = nullptr;

	// The index of this attributes in the owning component's attribute array, if valid.
	int32 Index = INDEX_NONE;

	// the identifier of this attribute on the owning component. We use this to check validity with the attribute in the given index.
	int32 Identifier = INDEX_NONE;
};