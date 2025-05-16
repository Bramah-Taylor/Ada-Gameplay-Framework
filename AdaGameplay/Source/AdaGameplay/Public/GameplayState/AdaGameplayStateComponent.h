// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "GameFramework/AdaGameplayTagCountContainer.h"

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

	friend struct FAdaAttributeHandle;
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
	FAdaAttributeHandle AddAttribute(const FGameplayTag AttributeTag, const FAdaAttributeInitParams& InitParams);

	/// @brief	Remove the provided attribute from this component.
	/// @param	AttributeHandle	The attribute to remove.
	/// @warning This remains untested. Use at your own risk.
	void RemoveAttribute(const FAdaAttributeHandle& AttributeHandle);

	/// @brief Find the given attribute on this component.
	/// @param	AttributeTag	The attribute to find.
	/// @return A pointer to the attribute. Will be null if the attribute is not found.
	FAdaAttributeHandle FindAttribute(const FGameplayTag AttributeTag) const;

	/// @brief	Query if an attribute exists on this component.
	/// @param	AttributeTag	The attribute to query for.
	/// @return Whether this component has the attribute or not.
	bool HasAttribute(const FGameplayTag AttributeTag) const;

	/// @brief	Get the value of an attribute on this component.
	/// @param	AttributeTag	The attribute to query for.
	/// @return The value of the attribute. Will be 0 if the attribute does not exist.
	float GetAttributeValue(const FGameplayTag AttributeTag) const;

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

	/// @brief	Attempt to find a status effect on this component by handle.
	/// @param	StatusEffectHandle	The handle to the status effect we want to find on this component.
	/// @return A pointer to the status effect. Will be null if invalid.
	/// @note	The returned pointer will be null if the status effect is not found.
	const UAdaStatusEffect* FindStatusEffect(const FAdaStatusEffectHandle& StatusEffectHandle) const;

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
	FORCEINLINE const TSparseArray<FAdaAttribute>& GetAllAttributes() const { return Attributes; };
	FORCEINLINE const TSparseArray<FAdaAttributeModifier>& GetAllModifiers() const { return ActiveModifiers; };
	FORCEINLINE const FAdaGameplayTagCountContainer& GetActiveState() const { return ActiveStates; };

protected:
	// Begin UActorComponent overrides.
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UActorComponent overrides.

	void FixedTick(const uint64& CurrentTick);

	// Utility functions for finding attributes on this component.
	FAdaAttribute* FindAttribute_Internal(const FGameplayTag AttributeTag);
	const FAdaAttribute* FindAttribute_Internal(const FGameplayTag AttributeTag) const;

	FAdaAttribute* FindAttributeByIndex(int32 Index);
	const FAdaAttribute* FindAttributeByIndex(int32 Index) const;

	// Utility functions for finding attribute modifiers by their array index.
	FAdaAttributeModifier* FindModifierByIndex(int32 Index);
	const FAdaAttributeModifier*FindModifierByIndex(int32 Index) const;

	// Utility function for removing a modifier we know the index of.
	bool RemoveModifierByIndex(int32 Index);

	// Remove the specified modifier from all references on this component. That includes the modifier array,
	// any references to this modifier on attributes, and any attribute dependency references.
	bool RemoveModifier_Internal(FAdaAttributeModifier& Modifier, int32 Index);

	// Immediately apply an instant, permanent modifier to this attribute.
	void ApplyImmediateModifier(FAdaAttribute& Attribute, const FAdaAttributeModifierSpec& ModifierToApply);

	// Apply a modifier that overrides the given attribute.
	void ApplyOverridingModifier(FAdaAttribute& Attribute, const FAdaAttributeModifier& Modifier, const int32 ModifierIndex);

	// Recalculate the value of an attribute from its modifiers.
	void RecalculateAttribute(FAdaAttribute& Attribute, const uint64& CurrentTick);

	// Check if attribute A depends on attribute B. Used to prevent circular dependencies.
	bool DoesAttributeDependOnOther(const FGameplayTag AttributeTag, const FGameplayTag OtherAttributeTag) const;

	// Let attributes, effects and delegate subscribers know an attribute's value has changed.
	void NotifyAttributeChanged(FAdaAttribute& Attribute, const float OldBase, const float OldCurrent);

	// Get an identifier for a new attribute.
	// Designed to overflow and avoid the error case of INDEX_NONE.
	int32 GetNextAttributeId();

	// Get an identifier for a new modifier.
	// Designed to overflow and avoid the error case of INDEX_NONE.
	int32 GetNextModifierId();

	// Get an identifier for a new status effect.
	// Designed to overflow and avoid the error case of INDEX_NONE.
	int32 GetNextStatusEffectId();

	// Remove the specified status effect from this component.
	bool RemoveStatusEffect_Internal(const int32 Index);

protected:
	// #TODO(Ada.Gameplay.Optimisation): Reserve memory & define allocator?
	TSparseArray<FAdaAttribute> Attributes;

	// #TODO(Ada.Gameplay.Optimisation) TSparseArray has poorer performance for iteration due to non-contiguous allocation.
	// FAdaAttributeModifier is a nullable type and should be trivially relocatable, so we can bypass both the pointer and index
	// instability of TArray by wrapping it in a collection type that allocates and frees instances for us.
	// That would mean either never shrinking or implementing our own shrinking method.
	// A generic collection type for this would prove beneficial.
	TSparseArray<FAdaAttributeModifier> ActiveModifiers;

	// #TODO(Ada.Gameplay.Optimisation) TSparseArray has poorer performance for iteration due to non-contiguous allocation.
	// See above, use a collection of UObject pointers and then we can do away with TStrongObjectPtr.
	TSparseArray<TStrongObjectPtr<UAdaStatusEffect>> ActiveStatusEffects;

	FAdaGameplayTagCountContainer ActiveStates;
	FAdaGameplayTagCountContainer ActiveStatusEffectTags;

	uint64 LatestTick = 0;
	int32 LatestModifierId = 0;
	int32 LatestStatusEffectId = 0;
};