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
	float MaxValue = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MinValue = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bUsesClamping = false;
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
	FORCEINLINE float GetMaxValue() const { return MaxValue; };
	FORCEINLINE float GetMinValue() const { return MinValue; };
	FORCEINLINE float GetGrowthRate() const { return GrowthRate; };
	FORCEINLINE float GetModifierCount() const { return ActiveModifiers.Num(); };
	FORCEINLINE float GetDependencyCount() const { return AttributeDependencies.Num(); };

public:
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AttributeTag = FGameplayTag::EmptyTag;

	FAdaOnAttributeUpdated OnAttributeUpdated;
	
protected:
	UPROPERTY(BlueprintReadOnly)
	float ResetValue = 0.0f;
	
	// The persistent value of this attribute.
	UPROPERTY(BlueprintReadOnly)
	float BaseValue = 0.0f;

	// The value of this attribute after temporary modifiers have been calculated and applied.
	UPROPERTY(BlueprintReadOnly)
	float CurrentValue = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float MaxValue = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float MinValue = 0.0f;

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