// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "AdaAttributeModifierTypes.generated.h"

struct FAdaAttribute;
class UAdaGameplayStateComponent;

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
	FAdaAttributeModifier(const FGameplayTag Attribute, const FAdaAttributeModifierSpec& ModifierSpec, const uint64& CurrentFrame, const int32 NewId);

	FORCEINLINE FGameplayTag GetAffectedAttribute() const { return AffectedAttribute; };
	FORCEINLINE float GetValue() const { return ModifierValue; };
	FORCEINLINE int32 GetIdentifier() const { return Identifier; };
	
	bool HasDuration() const;
	bool HasExpired(const uint64& CurrentFrame) const;
	bool ModifiesClamping() const;
	
	bool ShouldRecalculate() const;
	bool CanApply(const uint64& CurrentFrame);

	float CalculateValue();
	void SetValue(float NewValue);

	void PostApply(const uint64& CurrentFrame);
	
	FString ToString() const;

protected:
	void SetModifyingAttribute(const FAdaAttribute& InAttribute);

	FGameplayTag AffectedAttribute = FGameplayTag::EmptyTag;
	FGameplayTag ModifyingAttribute = FGameplayTag::EmptyTag;
	
	EAdaAttributeModApplicationType ApplicationType = EAdaAttributeModApplicationType::Instant;
	EAdaAttributeModCalcType CalculationType = EAdaAttributeModCalcType::SetByCaller;
	EAdaAttributeModOpType OperationType = EAdaAttributeModOpType::Additive;

	bool bAffectsBase = false;
	float ModifierValue = 0.0f;

private:
	// Struct for holding params for optional set by delegate recalculation.
	FAdaAttributeModifierDelegate ModifierDelegate;

	// Struct holding params for optional clamping modification.
	FAdaAttributeModifierClampingParams ClampingParams;

	// The frame on which this modifier was first applied.
	// Relevant to duration-based modifiers.
	uint64 StartFrame = 0;

	// The most recent frame on which this modifier was applied.
	// Relevant to tick-based modifiers.
	uint64 LastApplicationFrame = 0;

	// The interval, in ticks, for when we should apply this modifier.
	// Relevant to tick-based modifiers.
	uint8 Interval = 0;

	// How many ticks we should apply this modifier for.
	// Relevant to duration-based modifiers.
	uint32 Duration = 0;

	// The identifier for this modifier. Used to check handle validity when other systems wish to access this modifier.
	int32 Identifier = INDEX_NONE;

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