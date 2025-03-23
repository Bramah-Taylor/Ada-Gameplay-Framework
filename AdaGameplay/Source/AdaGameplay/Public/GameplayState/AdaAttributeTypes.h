// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "AdaAttributeTypes.generated.h"

struct FAdaAttributeModifier;
class UAdaGameplayStateComponent;

DECLARE_MULTICAST_DELEGATE_FiveParams(FAdaOnAttributeUpdated, const FGameplayTag /*AttributeTag*/, const float /*NewBase*/, const float /*NewCurrent*/, const float /*OldBase*/, const float /*OldCurrent*/);

USTRUCT(BlueprintType)
struct ADAGAMEPLAY_API FAdaAttributeInitParams
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float InitialValue = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bUsesClamping = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (EditCondition = "bUsesClamping", EditConditionHides))
	FVector2D InitialClampingValues = FVector2D::ZeroVector;
};

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
	FORCEINLINE float GetGrowthRate() const { return GrowthRate; };
	FORCEINLINE float GetModifierCount() const { return ActiveModifiers.Num(); };
	FORCEINLINE float GetDependencyCount() const { return AttributeDependencies.Num(); };

public:
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AttributeTag = FGameplayTag::EmptyTag;

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

	UPROPERTY(BlueprintReadOnly)
	FVector2D BaseClampingValues = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector2D CurrentClampingValues = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	bool bUsesClamping = false;

	UPROPERTY(BlueprintReadOnly)
	float GrowthRate = 0.0f;

private:
	TArray<TWeakPtr<FAdaAttributeModifier>> ActiveModifiers;
	TWeakPtr<FAdaAttributeModifier> OverridingModifier = nullptr;

	TMap<FGameplayTag, int32> AttributeDependencies;

	bool bIsDirty = false;
	bool bIsOverridden = false;
};

USTRUCT()
struct ADAGAMEPLAY_API FAdaAttributeHandle
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	FAdaAttributeHandle() = default;
	FAdaAttributeHandle(const TSharedRef<FAdaAttribute>& InAttribute);
	
	bool IsValid() const;

	const FAdaAttribute* Get() const;

public:
	FGameplayTag AttributeTag = FGameplayTag::EmptyTag;

private:
	void Invalidate();

private:
	TWeakPtr<FAdaAttribute> AttributeWeak = nullptr;
};