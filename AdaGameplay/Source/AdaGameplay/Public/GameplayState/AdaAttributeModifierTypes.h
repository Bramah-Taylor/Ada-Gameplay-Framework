// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Data/AdaTaggedTableRow.h"

#include "AdaAttributeModifierTypes.generated.h"

class UAdaStatusEffect;
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
	SetByDelegate		UMETA(Tooltip = "Dynamic modifier, recalculated based on the provided delegate", Hidden),
	SetByEffect			UMETA(Tooltip = "Dynamic modifier, recalculated based on the owning gameplay effect"),
	SetByData			UMETA(Tooltip = "Dynamic modifier, recalculated based on the provided float curve"),
	SetByAttribute		UMETA(Tooltip = "Dynamic modifier, recalculated based on the modifying attribute"),
	SetExternally		UMETA(Tooltip = "Dynamic modifier, set externally by some other system", Hidden)
};

UENUM()
enum class EAdaAttributeModOpType : uint8
{
	Override,
	Additive,
	Multiply,
	PostAdditive
};

// Struct representing changes to an attribute's clamping parameters.
// This works as an additive change to the min, max, or both. Whether it affects the base or current clamping values
// will depend on the type of modifier being applied.
USTRUCT(BlueprintType)
struct FAdaAttributeModifierClampingParams
{
	GENERATED_BODY()

public:
	// Whether this modifier changes the attribute's clamping parameters.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clamping")
	bool bActive = false;

	// Whether this modifier changes the attribute's minimum clamping parameter.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clamping", Meta = (EditCondition = "bActive", EditConditionHides))
	bool bHasMinDelta = false;

	// The amount by which to change the minimum clamping parameter.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clamping", Meta = (EditCondition = "bHasMinDelta", EditConditionHides))
	float MinDelta = 0.0f;

	// Whether this modifier changes the attribute's maximum clamping parameter.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clamping", Meta = (EditCondition = "bActive", EditConditionHides))
	bool bHasMaxDelta = false;

	// The amount by which to change the maximum clamping parameter.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clamping", Meta = (EditCondition = "bHasMaxDelta", EditConditionHides))
	float MaxDelta = 0.0f;
};

USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeModifierDelegate
{
	GENERATED_BODY()

	using FShouldRecalculateDelegate = TFunction<bool(const FGameplayTag)>;
	using FRecalculateDelegate = TFunction<float(const FGameplayTag)>;

public:
	bool bIsSet = false;

	FShouldRecalculateDelegate ShouldRecalculateModifierFunc;
	FRecalculateDelegate RecalculateModifierFunc;

	TWeakObjectPtr<UObject> InvokingObject = nullptr;
};

// Struct defining initialization params for an attribute modifier.
// This limits exposure to the actual live data used by the struct and hands full control of initialization over to the attribute system.
USTRUCT(BlueprintType)
struct ADAGAMEPLAY_API FAdaAttributeModifierSpec
{
	GENERATED_BODY()

	friend struct FAdaAttributeModifier;
	friend class UAdaGameplayStateComponent;
	friend class UAdaAttributeFunctionLibrary;
	friend class UAdaStatusEffectDefinition;

public:
	void SetPeriodicData(uint8 InInterval, uint32 InDuration, bool bApplyOnAdd, bool bApplyOnRemoval);
	void SetDurationData(uint32 InDuration);
	void SetClampingParams(TOptional<float> MinDelta, TOptional<float> MaxDelta);
	void SetDelegate(const FAdaAttributeModifierDelegate& Delegate);
	void SetCurveData(const FGameplayTag CurveTag, const float InCurveSpeed, const float InCurveMultiplier);
	void SetEffectData(UAdaStatusEffect* StatusEffect);
	
	bool ModifiesClamping() const;
	bool AffectsBaseValue() const;

public:
	// How, and when, we apply this modifier to the target attribute.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EAdaAttributeModApplicationType ApplicationType = EAdaAttributeModApplicationType::Instant;

	// How, and when, we calculate the value for this modifier.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EAdaAttributeModCalcType CalculationType = EAdaAttributeModCalcType::SetByCaller;

	// How this modifier's contribution to the target attribute's final value is calculated.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EAdaAttributeModOpType OperationType = EAdaAttributeModOpType::Additive;

	// The attribute to use as the value for our modification.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "CalculationType==EAdaAttributeModCalcType::SetByAttribute", EditConditionHides))
	FGameplayTag ModifyingAttribute = FGameplayTag::EmptyTag;

	// The value this modifier will alter the target attribute by.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "CalculationType!=EAdaAttributeModCalcType::SetByAttribute", EditConditionHides))
	float ModifierValue = 0.0f;

	// Whether to recalculate the target attribute as soon as this modifier is applied.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "ApplicationType!=EAdaAttributeModApplicationType::Instant", EditConditionHides))
	bool bRecalculateImmediately = false;

protected:
	// The interval, in ticks, for when this modifier should be applied to the target attribute.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "ApplicationType==EAdaAttributeModApplicationType::Periodic", EditConditionHides, ClampMin = 1))
	uint8 Interval = 0;

	// The total duration, in ticks, for how long this modifier is active.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "ApplicationType!=EAdaAttributeModApplicationType::Instant && ApplicationType!=EAdaAttributeModApplicationType::Persistent", EditConditionHides, ClampMin = 1))
	int32 Duration = 0;

	// Whether to apply this modifier to the target attribute once it has expired.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "ApplicationType==EAdaAttributeModApplicationType::Periodic", EditConditionHides))
	bool bShouldApplyOnRemoval = false;

	// The tag for the curve modifier that we're using to lookup values from.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "CalculationType==EAdaAttributeModCalcType::SetByData", EditConditionHides))
	FGameplayTag ModifierCurveTag = FGameplayTag::EmptyTag;

	// How quickly this data curve based modifier should progress through the curve, from 0 to 1.
	// 1 will default to 100% speed, which means the curve will have been fully traversed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "CalculationType==EAdaAttributeModCalcType::SetByData", EditConditionHides))
	float CurveSpeed = 0.1f;

	// Multiplier for the output value from the curve lookup.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "CalculationType==EAdaAttributeModCalcType::SetByData", EditConditionHides))
	float CurveMultiplier = 100.0f;

	// Struct holding params for optional clamping modification.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FAdaAttributeModifierClampingParams ClampingParams;

	TWeakObjectPtr<UAdaStatusEffect> ParentStatusEffect = nullptr;

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

	inline FGameplayTag GetAffectedAttribute() const { return AffectedAttribute; };
	inline float GetValue() const { return ModifierValue; };
	inline int32 GetIdentifier() const { return Identifier; };
	
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

	// #TODO: Replace with attribute handles?
	FGameplayTag AffectedAttribute = FGameplayTag::EmptyTag;
	FGameplayTag ModifyingAttribute = FGameplayTag::EmptyTag;
	
	EAdaAttributeModApplicationType ApplicationType = EAdaAttributeModApplicationType::Instant;
	EAdaAttributeModCalcType CalculationType = EAdaAttributeModCalcType::SetByCaller;
	EAdaAttributeModOpType OperationType = EAdaAttributeModOpType::Additive;

	bool bAffectsBase = false;
	float ModifierValue = 0.0f;

private:
	// Cached curve float to look up values for curve modifiers.
	TWeakObjectPtr<const UCurveFloat> ModifierCurve = nullptr;

	// The status effect that owns this modifier, if it was created by a status effect's activation.
	TWeakObjectPtr<UAdaStatusEffect> ParentStatusEffect = nullptr;
	
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
	FAdaAttributeModifierHandle(UAdaGameplayStateComponent* const Owner, const int32 NewIndex, const int32 NewId);
	
	bool IsValid(bool bValidateOwner = false) const;
	void Invalidate();

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
struct ADAGAMEPLAY_API FAdaAttributeCurveModifierRow : public FAdaTaggedTableRow
{
	GENERATED_BODY()

public:
	// Begin FAdaTaggedTableRow overrides.
	virtual FGameplayTag GetRowTag() const override { return CurveTag; };
	// End FAdaTaggedTableRow overrides.

public:
	// The tag that identifies this curve modifier.
	UPROPERTY(EditDefaultsOnly, Category = "Data", Meta = (Categories = "Modifier.Curve"))
	FGameplayTag CurveTag = FGameplayTag::EmptyTag;

	// The actual curve we'll use to look up values for our modifier.
	UPROPERTY(EditDefaultsOnly, Category = "Data")
	TObjectPtr<UCurveFloat> Curve = nullptr;
};