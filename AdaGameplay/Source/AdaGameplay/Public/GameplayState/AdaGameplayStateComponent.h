// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "GameplayState/AdaAttributeTypes.h"
#include "GameplayState/AdaAttributeModifierTypes.h"
#include "GameplayState/AdaGameplayEffect.h"

#include "AdaGameplayStateComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdaGameplayState, Log, All);

UCLASS(ClassGroup = (Custom), Meta = (BlueprintSpawnableComponent))
class ADAGAMEPLAY_API UAdaGameplayStateComponent : public UActorComponent
{
	GENERATED_BODY()

	friend struct FAdaAttributeModifierHandle;
	friend class UAdaAttributeFunctionLibrary;

public:	
	UAdaGameplayStateComponent();

	void FixedTick(const uint64& CurrentFrame);

	FAdaAttributePtr AddAttribute(const FGameplayTag AttributeTag, const FAdaAttributeInitParams& InitParams);
	void RemoveAttribute(const FGameplayTag AttributeTag);

	FAdaAttributePtr FindAttribute(const FGameplayTag AttributeTag) const;
	void InvalidateHandle(FAdaAttributePtr& InHandle) const;

	bool HasAttribute(const FGameplayTag AttributeTag) const;

	FAdaOnAttributeUpdated* GetDelegateForAttribute(const FGameplayTag AttributeTag);

	FAdaAttributeModifierHandle ModifyAttribute(const FGameplayTag AttributeTag, const FAdaAttributeModifierSpec& ModifierToApply);

	// Remove the modifier. Will cause a pending update on tick and won't reapply on removal.
	bool RemoveModifier(FAdaAttributeModifierHandle& ModifierHandle);

	// #TODO(Ada.Gameplay): Move to child class in game module
	FORCEINLINE const TArray<TSharedRef<FAdaAttribute>>& GetAllAttributes() const { return Attributes; };
	FORCEINLINE const TSparseArray<TSharedRef<FAdaAttributeModifier>>& GetAllModifiers() const { return ActiveModifiers; };

protected:
	// Begin UActorComponent overrides.
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UActorComponent overrides.
	
	TSharedPtr<FAdaAttribute> FindAttribute_Internal(const FGameplayTag AttributeTag);
	const TSharedPtr<FAdaAttribute> FindAttribute_Internal(const FGameplayTag AttributeTag) const;
	TOptional<TSharedRef<FAdaAttribute>> FindAttributeRef_Internal(const FGameplayTag AttributeTag);
	
	TOptional<TSharedRef<FAdaAttributeModifier>> FindModifierByIndex(int32 Index);
	const TOptional<TSharedRef<FAdaAttributeModifier>> FindModifierByIndex(int32 Index) const;

	bool RemoveModifierByIndex(int32 Index);
	bool RemoveModifier_Internal(TSharedRef<FAdaAttributeModifier>& Modifier, int32 Index);

	void ApplyImmediateModifier(FAdaAttribute& Attribute, const FAdaAttributeModifierSpec& ModifierToApply);
	void ApplyOverridingModifier(FAdaAttribute& Attribute, const TSharedRef<FAdaAttributeModifier>& Modifier);

	void RecalculateAttribute(FAdaAttribute& Attribute, const uint64& CurrentFrame);
	
	bool DoesAttributeDependOnOther(const FGameplayTag AttributeTag, const FGameplayTag OtherAttributeTag) const;

	void NotifyAttributeChanged(FAdaAttribute& Attribute, const float OldBase, const float OldCurrent);

	int32 GetNextModifierId();

	FAdaAttributePtr MakeAttributeHandle(const TSharedRef<FAdaAttribute>& InAttribute) const;

protected:
	// #TODO(Ada.Gameplay): Reserve memory & define allocator?
	TArray<TSharedRef<FAdaAttribute>> Attributes;

	TSparseArray<TSharedRef<FAdaAttributeModifier>> ActiveModifiers;

	uint64 LatestFrame = 0;
	int32 LatestModifierId = 0;
};