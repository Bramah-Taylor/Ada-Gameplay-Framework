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
	SetByCaller,		// Static modifier, doesn't change
	SetByDelegate,
	SetByEffect,
	SetByData,
	SetByAttribute,
	SetExternally		// Dynamic modifier, but the calculation is handled by the caller
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
struct ADAGAMEPLAY_API FAdaAttributeModifierSpec
{
	GENERATED_BODY()

	friend struct FAdaAttributeModifier;

public:
	void SetPeriodicData(uint8 InInterval, uint32 InDuration, bool bApplyOnAdd, bool bApplyOnRemoval);
	void SetDurationData(uint32 InDuration);

public:
	EAdaAttributeModApplicationType ApplicationType = EAdaAttributeModApplicationType::Instant;
	EAdaAttributeModCalcType CalculationType = EAdaAttributeModCalcType::SetByCaller;
	EAdaAttributeModOpType OperationType = EAdaAttributeModOpType::Additive;
	
	FGameplayTag ModifyingAttribute = FGameplayTag::EmptyTag;
	
	float ModifierValue = 0.0f;

	bool bAffectsBase = false;
	bool bRecalculateImmediately = false;

private:
	uint8 Interval = 0;
	uint32 Duration = 0;
	
	bool bShouldApplyOnRemoval = false;

	// #TODO: Add support for changing clamping values.
};

// Modifiers are stored in different arrays based on their application type.
// #TODO: Figure out how to break this data down so that we're not shoving lots of unnecessary stuff into one massive struct.
USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeModifier
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	FAdaAttributeModifier() = default;
	FAdaAttributeModifier(const FGameplayTag Attribute, const FAdaAttributeModifierSpec& ModifierSpec, const uint64& CurrentFrame);

	bool HasDuration() const;
	bool HasExpired(const uint64& CurrentFrame) const;
	
	bool ShouldRecalculate();
	bool CanApply(const uint64& CurrentFrame);
	
	float CalculateValue();
	void SetValue(float NewValue);

	void PostApply(const uint64& CurrentFrame);

protected:
	void SetModifyingAttribute(const FAdaAttribute& InAttribute);

	// #TODO: Find a way of making these accessible to the Cog debug window without making them public.
public:
	FGameplayTag AffectedAttribute = FGameplayTag::EmptyTag;
	FGameplayTag ModifyingAttribute = FGameplayTag::EmptyTag;
	
	EAdaAttributeModApplicationType ApplicationType = EAdaAttributeModApplicationType::Instant;
	EAdaAttributeModCalcType CalculationType = EAdaAttributeModCalcType::SetByCaller;
	EAdaAttributeModOpType OperationType = EAdaAttributeModOpType::Additive;

	bool bAffectsBase = false;
	float ModifierValue = 0.0f;

private:
	// #TODO: Add TFunction for dynamic modifiers

	uint64 StartFrame = 0;
	uint64 LastApplicationFrame = 0;
	uint8 Interval = 0;
	uint32 Duration = 0;

	bool bShouldApplyOnAdd = false;
	bool bHasAppliedOnAdd = false;
	
	bool bShouldApplyOnRemoval = false;
};

USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeModifierHandle
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	FAdaAttributeModifierHandle() = default;
	FAdaAttributeModifierHandle(UAdaGameplayStateComponent* Owner, EAdaAttributeModApplicationType Type, int32 NewIndex);
	
	bool IsValid() const;
	void Invalidate();

	FAdaAttributeModifier* Get();
	const FAdaAttributeModifier* Get() const;

	bool Remove();
	
private:
	TWeakObjectPtr<UAdaGameplayStateComponent> OwningStateComponentWeak = nullptr;

	EAdaAttributeModApplicationType ApplicationType;
	int32 Index = -1;
};