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
};

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
	// #TODO(Ada.Gameplay): Add TFunction for dynamic modifiers

	FAdaAttributeModifierClampingParams ClampingParams;

	uint64 StartFrame = 0;
	uint64 LastApplicationFrame = 0;
	uint8 Interval = 0;
	uint32 Duration = 0;

	int32 Identifier = INDEX_NONE;

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
	FAdaAttributeModifierHandle(UAdaGameplayStateComponent* Owner, const EAdaAttributeModApplicationType Type, const int32 NewIndex, const int32 NewId);
	
	bool IsValid() const;
	void Invalidate();

	FAdaAttributeModifier* Get();
	const FAdaAttributeModifier* Get() const;

	bool Remove();
	
private:
	TWeakObjectPtr<UAdaGameplayStateComponent> OwningStateComponentWeak = nullptr;

	EAdaAttributeModApplicationType ApplicationType;
	int32 Index = INDEX_NONE;
	
	int32 Identifier = INDEX_NONE;
};