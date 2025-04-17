// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"

#include "GameplayState/AdaAttributeTypes.h"
#include "GameplayState/AdaAttributeModifierTypes.h"
#include "GameplayState/AdaStatusEffectDefinition.h"
#include "GameplayState/AdaStatusEffectTypes.h"

#include "AdaGameplayStateComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdaGameplayState, Log, All);

UCLASS(ClassGroup = (Custom), Meta = (BlueprintSpawnableComponent))
class ADAGAMEPLAY_API UAdaGameplayStateComponent : public UActorComponent
{
	GENERATED_BODY()

	friend struct FAdaAttributeModifierHandle;
	friend class UAdaAttributeFunctionLibrary;
	friend class UAdaGameplayStateManager;

public:	
	UAdaGameplayStateComponent();

	/// @brief	Add an attribute to this component with some initial data.
	/// @param	AttributeTag	The attribute we want to add.
	/// @param	InitParams		Parameters for setting the initial state of this attribute.
	/// @return A pointer to the attribute.
	/// @note	The returned pointer will be null if this attribute already exists on the component.
	FAdaAttributePtr AddAttribute(const FGameplayTag AttributeTag, const FAdaAttributeInitParams& InitParams);

	/// @brief	Remove the provided attribute from this component.
	/// @param	AttributeTag	The attribute to remove.
	/// @warning This remains untested. Use at your own risk.
	void RemoveAttribute(const FGameplayTag AttributeTag);

	/// @brief Find the given attribute on this component.
	/// @param	AttributeTag	The attribute to find.
	/// @return A pointer to the attribute. Will be null if the attribute is not found.
	FAdaAttributePtr FindAttribute(const FGameplayTag AttributeTag) const;

	/// @brief	Query if an attribute exists on this component.
	bool HasAttribute(const FGameplayTag AttributeTag) const;

	/// @brief	Get a delegate that broadcasts whenever the provided attribute is updated.
	/// @param	AttributeTag	The attribute we want to listen to changes for.
	/// @return A delegate to subscribe to for changes to this attribute.
	/// @note	The returned delegate will be null if the attribute is not found.
	FAdaOnAttributeUpdated* GetDelegateForAttribute(const FGameplayTag AttributeTag);

	/// @brief	Modify the given attribute using the provided modifier spec.
	///			This function will produce an attribute modifier, which this component manages internally based on the provided spec.
	/// @param	AttributeTag	The attribute we want to modify.
	/// @param	ModifierToApply	The modifier spec to apply as a modifier to this attribute.
	/// @return	Handle to the modifier to the affected attribute.
	/// @note	The handle will be invalid if the modifier failed to apply.
	FAdaAttributeModifierHandle ModifyAttribute(const FGameplayTag AttributeTag, const FAdaAttributeModifierSpec& ModifierToApply);

	/// @brief	Remove the modifier. Will cause a pending update on tick and won't reapply on removal.
	/// @param	ModifierHandle	Handle to the modifier we wish to remove.
	/// @return Whether the modifier was removed successfully.
	bool RemoveModifier(FAdaAttributeModifierHandle& ModifierHandle);

	/// @brief	Add a status effect to this component.
	/// @param	StatusEffectTag	The gameplay tag representing the status effect.
	/// @return Handle to the newly added status effect.
	/// @note	The handle will be invalid if the status effect failed to apply.
	FAdaStatusEffectHandle AddStatusEffect(const FGameplayTag StatusEffectTag);

	/// @brief	Remove a status effect from this component.
	/// @param	StatusEffectHandle	The handle to the status effect we want to remove from this component.
	/// @return Whether the status effect was removed successfully.
	bool RemoveStatusEffect(FAdaStatusEffectHandle& StatusEffectHandle);

	/// @brief	Clear all instances of a status effect from this component.
	/// @param	StatusEffectTag	The gameplay tag representing the status effect.
	/// @return Whether the status effect was cleared successfully.
	bool ClearStatusEffect(const FGameplayTag StatusEffectTag);

	/// @brief	Check if this component has the specified state tag.
	/// @param	StateTag		The tag to check.
	/// @param	bExactMatch		Whether the found tag must be exactly the same as the specified tag.
	/// @return Whether this component has the specified tag.
	bool HasState(const FGameplayTag StateTag, bool bExactMatch = true) const;

	/// @brief	Check if this component has any of the specified state tags.
	/// @param	StateTags		The tags to check.
	/// @param	bExactMatch		Whether the found tags must be exactly the same as the specified tags.
	/// @return Whether this component has any of the specified tags.
	bool HasAnyState(const FGameplayTagContainer& StateTags, bool bExactMatch = true) const;

	/// @brief	Check if this component has all of the specified state tags.
	/// @param	StateTags		The tags to check.
	/// @param	bExactMatch		Whether the found tags must be exactly the same as the specified tags.
	/// @return Whether this component has all of the specified tags.
	bool HasAllState(const FGameplayTagContainer& StateTags, bool bExactMatch = true) const;

	/// @brief	Add a tag representing some state to this component.
	/// @param	StateTag		The tag to add.
	/// @return Whether this component successfully added the tag.
	bool AddStateTag(const FGameplayTag StateTag);

	/// @brief	Remove a tag representing some state to this component.
	/// @param	StateTag		The tag to add.
	/// @return Whether this component successfully removed the tag.
	bool RemoveStateTag(const FGameplayTag StateTag);

	// #TODO(Ada.Gameplay): Move to child class in game module
	FORCEINLINE const TArray<TSharedRef<FAdaAttribute>>& GetAllAttributes() const { return Attributes; };
	FORCEINLINE const TSparseArray<TSharedRef<FAdaAttributeModifier>>& GetAllModifiers() const { return ActiveModifiers; };

protected:
	// Begin UActorComponent overrides.
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UActorComponent overrides.

	void FixedTick(const uint64& CurrentTick);

	// Utility functions for finding attributes on this component.
	TSharedPtr<FAdaAttribute> FindAttribute_Internal(const FGameplayTag AttributeTag);
	const TSharedPtr<FAdaAttribute> FindAttribute_Internal(const FGameplayTag AttributeTag) const;

	// Specific utility function for finding an attribute when we need to use the actual shared ref instead of a pointer.
	TOptional<TSharedRef<FAdaAttribute>> FindAttributeRef_Internal(const FGameplayTag AttributeTag);

	// Utility functions for finding attribute modifiers by their array index.
	TOptional<TSharedRef<FAdaAttributeModifier>> FindModifierByIndex(int32 Index);
	const TOptional<TSharedRef<FAdaAttributeModifier>> FindModifierByIndex(int32 Index) const;

	// Utility function for removing a modifier we know the index of.
	bool RemoveModifierByIndex(int32 Index);

	// Remove the specified modifier from all references on this component. That includes the modifier array,
	// any references to this modifier on attributes, and any attribute dependency references.
	bool RemoveModifier_Internal(TSharedRef<FAdaAttributeModifier>& Modifier, int32 Index);

	// Immediately apply an instant, permanent modifier to this attribute.
	void ApplyImmediateModifier(FAdaAttribute& Attribute, const FAdaAttributeModifierSpec& ModifierToApply);

	// Apply a modifier that overrides the given attribute.
	void ApplyOverridingModifier(FAdaAttribute& Attribute, const TSharedRef<FAdaAttributeModifier>& Modifier);

	// Recalculate the value of an attribute from its modifiers.
	void RecalculateAttribute(FAdaAttribute& Attribute, const uint64& CurrentTick);

	// Check if attribute A depends on attribute B. Used to prevent circular dependencies.
	bool DoesAttributeDependOnOther(const FGameplayTag AttributeTag, const FGameplayTag OtherAttributeTag) const;

	// Let attributes, effects and delegate subscribers know an attribute's value has changed.
	void NotifyAttributeChanged(FAdaAttribute& Attribute, const float OldBase, const float OldCurrent);

	// Get an identifier for a new modifier.
	// Designed to overflow and avoid the error case of INDEX_NONE.
	int32 GetNextModifierId();

	// Make a shareable pointer to an attribute on this component.
	FAdaAttributePtr MakeAttributePtr(const TSharedRef<FAdaAttribute>& InAttribute) const;

protected:
	// #TODO(Ada.Gameplay): Reserve memory & define allocator?
	TArray<TSharedRef<FAdaAttribute>> Attributes;

	TSparseArray<TSharedRef<FAdaAttributeModifier>> ActiveModifiers;
	
	TSparseArray<TStrongObjectPtr<UAdaStatusEffectDefinition>> ActiveStatusEffects;

	FGameplayTagContainer ActiveStates;

	uint64 LatestTick = 0;
	int32 LatestModifierId = 0;
};