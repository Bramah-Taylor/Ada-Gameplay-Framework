// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "AdaAttributeModifierTypes.generated.h"

struct FAdaAttribute;
class UAdaGameplayStateComponent;

// Modifiers come in many different varieties, and as such have a wide variety of behaviours based on combinations of their
// application type and calculation type. Below is a description of how the different types are intended to be used.
//
//		-- Application Types --
// Instant:		Used for permanent changes to an attribute.
//				For example, an additive modifier with a modifier value of 10.0f will increase the base value of an attribute by 10.0f.
//
// Duration:	Used to temporarily change an attribute for a given duration.
//				The modifier will change the attribute's current value by the modifier's modifier value when applied.
//				For example, a multiplicative modifier with a value of 0.5f will halve the current value of an attribute until this modifier expires.
//
// Periodic:	Used to permanently alter an attribute by a given value at a set interval for a duration.
//				This modifier will change an attribute's base value by the given amount every time we hit the specified interval during the duration.
//				For example, an additive modifier with a value of -5.0f, an interval of 5 and a duration of 50 will reduce the base value of an attribute
//				by 5.0f every 5 ticks until 50 ticks have happened, with the option of triggering on either 0 (addition), 50 (removal), or both.
//
// Ticking:		Conceptually the same as a periodic modifier, except it applies every tick instead of on an interval.
//				For example, an additive modifier of 3.0f will increment the base value of an attribute by 3 each tick until the duration is hit.
//
// Persistent:	Used to temporarily modify the value of an attribute until this modifier is removed.
//				For example, a multiplicative value of 0.8f will reduce an attribute by 20% until the modifier is manually removed.
//
//		-- Calculation Types --
// SetByCaller:		Modifier value is static, being set once upon creation of the modifier.
//
// SetByDelegate:	Modifier value is set whenever the provided delegate is invoked.
//
// SetByEffect:		Modifier value is set by the status effect that owns this modifier. The rules around this are defined by the status effect.
//
// SetByData:		Modifier value is set dynamically by looking up a value on a curve.
//
// SetByAttribute:	Modifier value is set dynamically relative to another attribute's value.
//
// SetExternally:	Modifier value is set and updated manually by some external system.

UENUM()
enum class EAdaAttributeModApplicationType : uint8
{
	Instant,
	Duration,
	Periodic,
	Ticking,
	Persistent
};

UENUM()
enum class EAdaAttributeModCalcType : uint8
{
	SetByCaller			UMETA(Tooltip = "Static modifier set on creation of the modifier"),
	SetByDelegate		UMETA(Tooltip = "Dynamic modifier, recalculated based on the provided delegate"),
	SetByEffect			UMETA(Tooltip = "Dynamic modifier, recalculated based on the owning gameplay effect"),
	SetByData			UMETA(Tooltip = "Dynamic modifier, recalculated based on the provided float curve"),
	SetByAttribute		UMETA(Tooltip = "Dynamic modifier, recalculated based on the modifying attribute"),
	SetExternally		UMETA(Tooltip = "Dynamic modifier, set externally by some other system")
};

UENUM()
enum class EAdaAttributeModOpType : uint8
{
	Override,
	Additive,
	Multiply,
	PostAdditive
};

USTRUCT()
struct FAdaAttributeModifierClampingParams
{
	GENERATED_BODY()

public:
	bool bActive = false;
	bool bHasMinDelta = false;
	bool bHasMaxDelta = false;
	
	float MinDelta = 0.0f;
	float MaxDelta = 0.0f;
};

USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeModifierDelegate
{
	GENERATED_BODY()

public:
	bool bIsSet = false;

	TFunction<bool(const FGameplayTag)> ShouldRecalculateModifierFunc;
	TFunction<float(const FGameplayTag)> RecalculateModifierFunc;

	TWeakObjectPtr<UObject> InvokingObject = nullptr;
};

// Struct defining initialization params for an attribute modifier.
// This limits exposure to the actual live data used by the struct and hands full control of initialization over to the attribute system.
USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeModifierSpec
{
	GENERATED_BODY()

	friend struct FAdaAttributeModifier;
	friend class UAdaGameplayStateComponent;
	friend class UAdaAttributeFunctionLibrary;

public:
	void SetPeriodicData(uint8 InInterval, uint32 InDuration, bool bApplyOnAdd, bool bApplyOnRemoval);
	void SetDurationData(uint32 InDuration);
	void SetClampingParams(TOptional<float> MinDelta, TOptional<float> MaxDelta);
	void SetDelegate(const FAdaAttributeModifierDelegate& Delegate);
	void SetCurveData(const FGameplayTag CurveTag, const float InCurveSpeed, const float InCurveMultiplier);
	
	bool ModifiesClamping() const;
	bool AffectsBaseValue() const;

public:
	EAdaAttributeModApplicationType ApplicationType = EAdaAttributeModApplicationType::Instant;
	EAdaAttributeModCalcType CalculationType = EAdaAttributeModCalcType::SetByCaller;
	EAdaAttributeModOpType OperationType = EAdaAttributeModOpType::Additive;
	
	FGameplayTag ModifyingAttribute = FGameplayTag::EmptyTag;
	
	float ModifierValue = 0.0f;
	
	bool bRecalculateImmediately = false;

private:
	uint8 Interval = 0;
	uint32 Duration = 0;
	
	bool bShouldApplyOnRemoval = false;

	FGameplayTag ModifierCurveTag = FGameplayTag::EmptyTag;
	float CurveSpeed = 0.1f;
	float CurveMultiplier = 100.0f;

	FAdaAttributeModifierClampingParams ClampingParams;
	FAdaAttributeModifierDelegate ModifierDelegate;
};

// Struct defining a single modification to a single attribute.
// This is a generic class that currently contains all the relevant data for all potential permutations of modifiers.
// Some of this data may be pulled out of this struct at a later point, should it prove to be beneficial to do so.
// For now, we just provide everything here and avoid any complexity that would come from a polymorphic implementation.
USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeModifier
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	FAdaAttributeModifier() = default;
	FAdaAttributeModifier(const FGameplayTag Attribute, const FAdaAttributeModifierSpec& ModifierSpec, const uint64& CurrentTick, const int32 NewId);

	FORCEINLINE FGameplayTag GetAffectedAttribute() const { return AffectedAttribute; };
	FORCEINLINE float GetValue() const { return ModifierValue; };
	FORCEINLINE int32 GetIdentifier() const { return Identifier; };
	
	bool HasDuration() const;
	bool HasExpired(const uint64& CurrentTick) const;
	bool ModifiesClamping() const;
	
	bool ShouldRecalculate() const;
	bool CanApply(const uint64& CurrentTick);

	float CalculateValue();
	void SetValue(float NewValue);

	void PostApply(const uint64& CurrentTick);
	
	FString ToString() const;

protected:
	void SetModifyingAttribute(const FAdaAttribute& InAttribute);
	void SetModifierCurve(const UCurveFloat* const InModifierCurve);

	FGameplayTag AffectedAttribute = FGameplayTag::EmptyTag;
	FGameplayTag ModifyingAttribute = FGameplayTag::EmptyTag;
	
	EAdaAttributeModApplicationType ApplicationType = EAdaAttributeModApplicationType::Instant;
	EAdaAttributeModCalcType CalculationType = EAdaAttributeModCalcType::SetByCaller;
	EAdaAttributeModOpType OperationType = EAdaAttributeModOpType::Additive;

	bool bAffectsBase = false;
	float ModifierValue = 0.0f;

private:
	// Cached curve float to look up values for curve modifiers.
	UPROPERTY()
	TWeakObjectPtr<const UCurveFloat> ModifierCurve = nullptr;
	
	// Struct for holding params for optional set by delegate recalculation.
	FAdaAttributeModifierDelegate ModifierDelegate;

	// Struct holding params for optional clamping modification.
	FAdaAttributeModifierClampingParams ClampingParams;

	// The tick on which this modifier was first applied.
	// Relevant to duration-based modifiers.
	uint64 StartTick = 0;

	// The most recent tick on which this modifier was applied.
	// Relevant to tick-based modifiers.
	uint64 LastApplicationTick = 0;

	// The interval, in ticks, for when we should apply this modifier.
	// Relevant to tick-based modifiers.
	uint8 Interval = 0;

	// How many ticks we should apply this modifier for.
	// Relevant to duration-based modifiers.
	uint32 Duration = 0;

	// The identifier for this modifier. Used to check handle validity when other systems wish to access this modifier.
	int32 Identifier = INDEX_NONE;

	// The tag for this attribute's modifier curve, if it uses a data curve.
	FGameplayTag ModifierCurveTag = FGameplayTag::EmptyTag;

	// How quickly this data curve based modifier should progress through the curve, from 0 to 1.
	// 1 will default to 100% speed, which means the curve will have been fully traversed.
	float CurveSpeed = 0.1f;

	// The multiplier to apply to the output value of a curve lookup.
	float CurveMultiplier = 100.0f;

	// Current progress through the curve for this modifier, if it uses a curve.
	float CurveProgress = 0.0f;

	// Whether this modifier should apply when it's first added.
	// Relevant to tick-based modifiers.
	bool bShouldApplyOnAdd = false;

	// Whether this modifier was applied when it was added.
	// Relevant to tick-based modifiers.
	bool bHasAppliedOnAdd = false;

	// Whether this modifier should apply when it's expired or removed.
	// Relevant to tick-based modifiers.
	bool bShouldApplyOnRemoval = false;
};

// Handle to an active attribute modifier.
// We create a modifier handle when modifying the given attribute; this modifier can then only be accessed externally via the generated handle.
USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeModifierHandle
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	FAdaAttributeModifierHandle() = default;
	FAdaAttributeModifierHandle(UAdaGameplayStateComponent* Owner, const EAdaAttributeModApplicationType Type, const int32 NewIndex, const int32 NewId);
	
	bool IsValid() const;
	void Invalidate();

	FAdaAttributeModifier* Get();
	const FAdaAttributeModifier* Get() const;

	bool Remove();
	
private:
	// The component that owns this modifier when it's valid.
	TWeakObjectPtr<UAdaGameplayStateComponent> OwningStateComponentWeak = nullptr;

	// The index in the owning component's modifier array for this modifier, when it's valid.
	int32 Index = INDEX_NONE;

	// The identifier assigned to this modifier.
	// We use this to ensure validity of accessed modifiers when accessing by index.
	int32 Identifier = INDEX_NONE;
};

// Data table row for modifiers, identified by gameplay tag.
// Data tables are used here because CurveTables fix the interpolation method for all curves to be the same, and we don't want that.
USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeCurveModifierRow : public FTableRowBase
{
	GENERATED_BODY()

	// The tag that identifies this curve modifier.
	UPROPERTY(EditDefaultsOnly, Category = "Data", Meta = (Categories = "Modifier.Curve"))
	FGameplayTag CurveTag = FGameplayTag::EmptyTag;

	// The actual curve we'll use to look up values for our modifier.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UCurveFloat> Curve = nullptr;
};