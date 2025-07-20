// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaGameplayStateComponent.h"

#include "GameFramework/AdaGameState.h"
#include "GameplayState/AdaGameplayStateManager.h"
#include "Debug/AdaAssertionMacros.h"
#include "GameplayState/AdaAttributeFunctionLibrary.h"
#include "GameplayState/AdaStatusEffect.h"

DEFINE_LOG_CATEGORY(LogAdaGameplayState);

UAdaGameplayStateComponent::UAdaGameplayStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAdaGameplayStateComponent::BeginPlay()
{
	Super::BeginPlay();

	const UWorld* const World = GetWorld();
	A_ENSURE_RET(IsValid(World), void());

	const AAdaGameState* const GameState = World->GetGameState<AAdaGameState>();
	A_ENSURE_RET(IsValid(GameState), void());

	UAdaGameplayStateManager* StateManager = GameState->GetGameplayStateManager();
	A_ENSURE_RET(IsValid(StateManager), void());

	StateManager->RegisterStateComponent(this);
}

void UAdaGameplayStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ON_SCOPE_EXIT
	{
		Super::EndPlay(EndPlayReason);
	};

	const UWorld* const World = GetWorld();
	A_ENSURE_RET(World, void());

	const AAdaGameState* const GameState = World->GetGameState<AAdaGameState>();
	A_ENSURE_RET(GameState, void());

	UAdaGameplayStateManager* StateManager = GameState->GetGameplayStateManager();
	A_ENSURE_RET(StateManager, void());

	StateManager->UnregisterStateComponent(this);
}

void UAdaGameplayStateComponent::FixedTick(const uint64& CurrentTick)
{
	const UWorld* const World = GetWorld();
	A_VALIDATE_OBJ(World, void());

	TArray<TPair<FAdaAttributeModifier&, uint32>> ExpiredModifiers;
	TArray<TPair<FAdaAttributeModifier&, uint32>> PostTick_ExpiredModifiers;

	// Maintain internal tick reference.
	LatestTick = CurrentTick;
	
	for (auto It = ActiveModifiers.CreateIterator(); It; ++It)
	{
		int32 Index = It.GetIndex();
		
		FAdaAttributeModifier& Modifier = *It;

		if (!Modifier.CanApply(CurrentTick))
		{
			continue;
		}

		// #TODO: Replace with index from handle?
		FAdaAttribute* FoundAttribute = FindAttribute_Internal(Modifier.AffectedAttribute);
		if (!FoundAttribute)
		{
			UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Invalid periodic modifier for attribute %s"), __FUNCTION__,
			       *Modifier.AffectedAttribute.ToString());
			ExpiredModifiers.Add({Modifier, Index});
			continue;
		}

		bool bTryRecalculate = true;
		if (Modifier.HasExpired(CurrentTick))
		{
			if (Modifier.bShouldApplyOnRemoval)
			{
				PostTick_ExpiredModifiers.Add({Modifier, Index});
			}
			else
			{
				ExpiredModifiers.Add({Modifier, Index});
				bTryRecalculate = false;
			}
		}

		// Update dynamic modifiers.
		bool bValueChanged = false;
		if (bTryRecalculate && Modifier.ShouldRecalculate())
		{
			const float OldValue = Modifier.GetValue();
			const float NewValue = Modifier.CalculateValue();

			bValueChanged = !FMath::IsNearlyEqual(OldValue, NewValue);
		}

		bool bMarkAttributeDirty = false;
		if (Modifier.ApplicationType == EAdaAttributeModApplicationType::Persistent)
		{
			bMarkAttributeDirty = bValueChanged;
		}
		else
		{
			bMarkAttributeDirty = true;
		}

		if (bMarkAttributeDirty)
		{
			FAdaAttribute& Attribute = *FoundAttribute;
			Attribute.bIsDirty = true;
		}
	}

	for (auto& [ExpiredModifier, Index] : ExpiredModifiers)
	{
		RemoveModifier_Internal(ExpiredModifier, Index);
	}

	// Update attributes.
	for (auto It = Attributes.CreateIterator(); It; ++It)
	{
		FAdaAttribute& Attribute = *It;
		if (Attribute.bIsDirty)
		{
			RecalculateAttribute(Attribute, CurrentTick);
		}
	}

	for (auto& [ExpiredModifier, Index] : PostTick_ExpiredModifiers)
	{
		RemoveModifier_Internal(ExpiredModifier, Index);
	}
}

FAdaAttributeHandle UAdaGameplayStateComponent::AddAttribute(const FGameplayTag AttributeTag, const FAdaAttributeInitParams& InitParams)
{
	if (FindAttribute_Internal(AttributeTag))
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Already added attribute %s to component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
		return FAdaAttributeHandle();
	}

	const int32 Identifier = GetNextAttributeId();
	const int32 Index = Attributes.Add(FAdaAttribute(AttributeTag, InitParams, Identifier));

	return FAdaAttributeHandle(this, AttributeTag, Index, Identifier);
}

void UAdaGameplayStateComponent::RemoveAttribute(const FAdaAttributeHandle& AttributeHandle)
{
	const FAdaAttribute* FoundAttribute = AttributeHandle.Get();
	if (!FoundAttribute)
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeHandle.AttributeTag.ToString(), *GetNameSafe(this));
		return;
	}

	for (auto& [DependentAttributeTag, Index]: FoundAttribute->AttributeDependencies)
	{
		RemoveModifierByIndex(Index);
	}
	
	Attributes.RemoveAt(AttributeHandle.Index);
}

FAdaAttributeHandle UAdaGameplayStateComponent::FindAttribute(const FGameplayTag AttributeTag) const
{
	for (auto It = Attributes.CreateConstIterator(); It; ++It)
	{
		const int32 Index = It.GetIndex();
		const FAdaAttribute& Attribute = *It;
		
		if (Attribute.AttributeTag == AttributeTag)
		{
			return FAdaAttributeHandle(this, AttributeTag, Index, Attribute.Identifier);
		}
	}

	UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
	return FAdaAttributeHandle();
}

bool UAdaGameplayStateComponent::HasAttribute(const FGameplayTag AttributeTag) const
{
	if (FindAttribute_Internal(AttributeTag))
	{
		return true;
	}
	
	return false;
}

float UAdaGameplayStateComponent::GetAttributeValue(const FGameplayTag AttributeTag) const
{
	const FAdaAttribute* FoundAttribute = FindAttribute_Internal(AttributeTag);
	return FoundAttribute ? FoundAttribute->CurrentValue : 0.0f;
}

FAdaOnAttributeUpdated* UAdaGameplayStateComponent::GetDelegateForAttribute(const FGameplayTag AttributeTag)
{
	FAdaAttribute* Attribute = FindAttribute_Internal(AttributeTag);
	if (!Attribute)
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
		return nullptr;
	}

	return &Attribute->OnAttributeUpdated;
}

FAdaAttributeModifierHandle UAdaGameplayStateComponent::ModifyAttribute(const FGameplayTag AttributeTag, const FAdaAttributeModifierSpec& ModifierToApply)
{
	FAdaAttributeModifierHandle OutHandle = FAdaAttributeModifierHandle();
	
	FAdaAttribute* FoundAttribute = FindAttribute_Internal(AttributeTag);
	if (!FoundAttribute)
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
		return OutHandle;
	}

	// Can't modify attributes that are currently overridden unless the override is removed first.
	if (FoundAttribute->bIsOverridden)
	{
		return OutHandle;
	}

	TArray<FString> Errors;
	if (!UAdaAttributeFunctionLibrary::IsModifierValid(ModifierToApply, Errors))
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Invalid modifier for attribute %s:"), __FUNCTION__, *AttributeTag.ToString());
		for (const FString& ErrorString : Errors)
		{
			UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: %s"), __FUNCTION__, *ErrorString);
		}
		
		return OutHandle;
	}

	FAdaAttribute& Attribute = *FoundAttribute;

	// Perform initial modification, forcing refresh of the attribute.
	if (ModifierToApply.ApplicationType == EAdaAttributeModApplicationType::Instant)
	{
		ApplyImmediateModifier(Attribute, ModifierToApply);
	}
	else
	{
		auto CacheModifier = [this, Attribute](const FAdaAttributeModifier& ModifierRef, FAdaAttribute& InAttribute, FAdaAttributeModifierHandle& OutHandle, const int32 ModifierId) -> int32
		{
			// Cache the modifier.
			int32 OutIndex = ActiveModifiers.Add(ModifierRef);

			A_ENSURE(OutIndex != INDEX_NONE);
			OutHandle = FAdaAttributeModifierHandle(this, OutIndex, ModifierId);

			InAttribute.ActiveModifiers.Add(OutHandle);

			return OutIndex;
		};
		
		const int32 ModifierId = GetNextModifierId();
		// Create the modifier, but don't cache it yet as we may have extra setup to do first.
		FAdaAttributeModifier* Modifier = new FAdaAttributeModifier(AttributeTag, ModifierToApply, LatestTick, ModifierId);
		
		int32 OutIndex = INDEX_NONE;
		if (Modifier->CalculationType == EAdaAttributeModCalcType::SetByAttribute)
		{
			FAdaAttribute* ModifyingAttribute = FindAttribute_Internal(ModifierToApply.ModifyingAttribute);
			if (!ModifyingAttribute)
			{
				UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *ModifierToApply.ModifyingAttribute.ToString(), *GetNameSafe(this));
				return OutHandle;
			}

			if (DoesAttributeDependOnOther(ModifierToApply.ModifyingAttribute, AttributeTag))
			{
				UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Attempted to create circular modifier dependency for attributes %s and %s!"), __FUNCTION__, *AttributeTag.ToString(), *ModifierToApply.ModifyingAttribute.ToString());
				return OutHandle;
			}
			
			Modifier->SetModifyingAttribute(*ModifyingAttribute);
			
			// Cache the modifier.
			OutIndex = CacheModifier(*Modifier, Attribute, OutHandle, ModifierId);
			
			ModifyingAttribute->AttributeDependencies.Add({AttributeTag, OutIndex});
		}
		else if (Modifier->CalculationType == EAdaAttributeModCalcType::SetByData)
		{
			const UWorld* const World = GetWorld();
			A_ENSURE_RET(World, OutHandle);

			const AAdaGameState* const GameState = World->GetGameState<AAdaGameState>();
			A_ENSURE_RET(GameState, OutHandle);

			UAdaGameplayStateManager* StateManager = GameState->GetGameplayStateManager();
			A_ENSURE_RET(StateManager, OutHandle);
			
			Modifier->SetModifierCurve(StateManager->GetCurveForModifier(ModifierToApply.ModifierCurveTag));
			Modifier->CalculateValue();

			// Cache the modifier.
			OutIndex = CacheModifier(*Modifier, Attribute, OutHandle, ModifierId);
		}
		else
		{
			// Cache the modifier.
			OutIndex = CacheModifier(*Modifier, Attribute, OutHandle, ModifierId);
		}

		if (ModifierToApply.OperationType == EAdaAttributeModOpType::Override)
		{
			ApplyOverridingModifier(Attribute, *Modifier, OutIndex);
		}
	}

	if (ModifierToApply.bRecalculateImmediately)
	{
		RecalculateAttribute(Attribute, LatestTick);
	}
	else
	{
		Attribute.bIsDirty = true;
	}

	return OutHandle;
}

bool UAdaGameplayStateComponent::RemoveModifier(FAdaAttributeModifierHandle& ModifierHandle)
{
	FAdaAttributeModifier* Modifier = FindModifierByIndex(ModifierHandle.Index);
	if (!Modifier)
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Failed to find modifier for handle %i, index %i."), __FUNCTION__, ModifierHandle.Identifier, ModifierHandle.Index);
		return false;
	}

	if (Modifier->GetIdentifier() != ModifierHandle.Identifier)
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Got identifier mismatch at index %i."), __FUNCTION__, ModifierHandle.Identifier);
		return false;
	}

	RemoveModifier_Internal(*Modifier, ModifierHandle.Index);
	ModifierHandle.Invalidate();
	
	return true;
}

FAdaStatusEffectHandle UAdaGameplayStateComponent::AddStatusEffect(const FGameplayTag StatusEffectTag)
{
	if (!StatusEffectTag.IsValid())
	{
		return FAdaStatusEffectHandle();
	}

	const UWorld* const World = GetWorld();
	A_ENSURE_RET(IsValid(World), FAdaStatusEffectHandle());

	const AAdaGameState* const GameState = World->GetGameState<AAdaGameState>();
	A_ENSURE_RET(IsValid(GameState), FAdaStatusEffectHandle());

	const UAdaGameplayStateManager* const StateManager = GameState->GetGameplayStateManager();
	A_ENSURE_RET(IsValid(StateManager), FAdaStatusEffectHandle());

	const UAdaStatusEffectDefinition* const StatusEffectDef = StateManager->GetStatusEffectDefinition(StatusEffectTag);
	A_ENSURE_RET(IsValid(StatusEffectDef), FAdaStatusEffectHandle());

	// Prevent activation if we already have an active instance of this effect and don't allow for stacking.
	if (ActiveStatusEffectTags.HasMatchingGameplayTag(StatusEffectTag) && !StatusEffectDef->bCanStack)
	{
		return FAdaStatusEffectHandle();
	}

	// Check we don't have any tags that would block this effect.
	if (ActiveStates.HasAnyMatchingGameplayTags(StatusEffectDef->BlockingTags))
	{
		return FAdaStatusEffectHandle();
	}

	// Check we have all of the tags that this effect requires for activation.
	if (!ActiveStates.HasAllMatchingGameplayTags(StatusEffectDef->EnablingTags))
	{
		return FAdaStatusEffectHandle();
	}

	const UClass* Implementation = IsValid(StatusEffectDef->Implementation) ? StatusEffectDef->Implementation.Get() : UAdaStatusEffect::StaticClass();
	TStrongObjectPtr<UAdaStatusEffect> NewStatusEffect = TStrongObjectPtr(NewObject<UAdaStatusEffect>(this, Implementation));
	A_ENSURE_RET(IsValid(NewStatusEffect.Get()), FAdaStatusEffectHandle());

	NewStatusEffect->EffectTag = StatusEffectTag;
	NewStatusEffect->EffectId = GetNextStatusEffectId();
	
	for (const auto& [AttributeTag, ModifierSpecRef] : StatusEffectDef->Modifiers)
	{
		// Copy the modifier spec from the effect definition.
		// We're going to modify the spec itself, so we don't want to propagate those changes into the
		// effect definition by mistake.
		FAdaAttributeModifierSpec EffectModifierSpec = ModifierSpecRef;
		EffectModifierSpec.SetEffectData(NewStatusEffect.Get());

		FAdaAttributeModifierHandle ModifierHandle = ModifyAttribute(AttributeTag, EffectModifierSpec);
		if (EffectModifierSpec.ApplicationType != EAdaAttributeModApplicationType::Instant)
		{
			NewStatusEffect->ActiveModifierHandles.Add(ModifierHandle);
		}
	}

	FAdaStatusEffectHandle NewStatusEffectHandle = FAdaStatusEffectHandle();
	NewStatusEffectHandle.OwningStateComponentWeak = this;
	NewStatusEffectHandle.Identifier = NewStatusEffect->EffectId;
	NewStatusEffectHandle.Index = ActiveStatusEffects.Add(MoveTemp(NewStatusEffect));

	// Add a new instance of the effect to our explicit tag tracking container.
	ActiveStatusEffectTags.UpdateTagCount(StatusEffectTag, 1);

	// Update the state on this component with the state from the effect.
	ActiveStates.UpdateTagCount(StatusEffectDef->StateTagsToAdd, 1);
	
	// Cancel any relevant effects
	if (!StatusEffectDef->EffectsToCancel.IsEmpty() || !StatusEffectDef->EffectTypesToCancel.IsEmpty())
	{
		TArray<int32> IndicesToRemove;
		for (auto It = ActiveStatusEffects.CreateConstIterator(); It; ++It)
		{
			int32 Index = It.GetIndex();
			const TStrongObjectPtr<UAdaStatusEffect> StatusEffectPtr = *It;
			A_ENSURE_RET(StatusEffectPtr, FAdaStatusEffectHandle());
			
			if (StatusEffectDef->EffectsToCancel.HasAny(StatusEffectPtr->EffectTag.GetSingleTagContainer()))
			{
				IndicesToRemove.Add(Index);
			}
			else if (StatusEffectDef->EffectTypesToCancel.HasAny(StatusEffectPtr->TagCategories))
			{
				IndicesToRemove.Add(Index);
			}
		}

		for (int32 RemoveIndices : IndicesToRemove)
		{
			ActiveStatusEffects.RemoveAt(RemoveIndices);
		}
	}
	
	return NewStatusEffectHandle;
}

bool UAdaGameplayStateComponent::RemoveStatusEffect(FAdaStatusEffectHandle& StatusEffectHandle)
{
	if (StatusEffectHandle.Identifier == INDEX_NONE || StatusEffectHandle.Index == INDEX_NONE)
	{
		return false;
	}

	const bool bSuccess = RemoveStatusEffect_Internal(StatusEffectHandle.Index);
	if (bSuccess)
	{
		StatusEffectHandle.Invalidate();
	}
	
	return bSuccess;
}

bool UAdaGameplayStateComponent::ClearStatusEffect(const FGameplayTag StatusEffectTag)
{
	TArray<int32> IndicesToRemove;
	for (auto It = ActiveStatusEffects.CreateConstIterator(); It; ++It)
	{
		int32 Index = It.GetIndex();
		const TStrongObjectPtr<UAdaStatusEffect> StatusEffectPtr = *It;
		
		const UAdaStatusEffect* const StatusEffect = StatusEffectPtr.Get();
		if (!A_ENSURE(IsValid(StatusEffect)))
		{
			continue;
		}

		if (StatusEffect->GetEffectTag() == StatusEffectTag)
		{
			IndicesToRemove.Add(Index);
		}
	}

	bool bSuccess = true;
	for (const int32 Index : IndicesToRemove)
	{
		bSuccess &= RemoveStatusEffect_Internal(Index);
	}
	
	return bSuccess;
}

const UAdaStatusEffect* UAdaGameplayStateComponent::FindStatusEffect(const FAdaStatusEffectHandle& StatusEffectHandle) const
{
	if (StatusEffectHandle.Identifier == INDEX_NONE || StatusEffectHandle.Index == INDEX_NONE)
	{
		return nullptr;
	}

	if (!ActiveStatusEffects.IsValidIndex(StatusEffectHandle.Index))
	{
		return nullptr;
	}

	const UAdaStatusEffect* const StatusEffect = ActiveStatusEffects[StatusEffectHandle.Index].Get();
	if (!IsValid(StatusEffect))
	{
		return nullptr;
	}
	
	return StatusEffect;
}

bool UAdaGameplayStateComponent::HasState(const FGameplayTag StateTag, bool bExactMatch) const
{
	const FGameplayTagContainer& ExplicitStateTags = ActiveStates.GetTags();
	return bExactMatch ? ExplicitStateTags.HasTagExact(StateTag) : ExplicitStateTags.HasTag(StateTag);
}

bool UAdaGameplayStateComponent::HasAnyState(const FGameplayTagContainer& StateTags, bool bExactMatch) const
{
	const FGameplayTagContainer& ExplicitStateTags = ActiveStates.GetTags();
	return bExactMatch ? ExplicitStateTags.HasAnyExact(StateTags) : ExplicitStateTags.HasAny(StateTags);
}

bool UAdaGameplayStateComponent::HasAllState(const FGameplayTagContainer& StateTags, bool bExactMatch) const
{
	const FGameplayTagContainer& ExplicitStateTags = ActiveStates.GetTags();
	return bExactMatch ? ExplicitStateTags.HasAllExact(StateTags) : ExplicitStateTags.HasAll(StateTags);
}

bool UAdaGameplayStateComponent::AddStateTag(const FGameplayTag StateTag)
{
	A_ENSURE_MSG_RET(StateTag.IsValid(), false, TEXT("%hs: Attempted to add invalid state tag."), __FUNCTION__);
	
	ActiveStates.UpdateTagCount(StateTag, 1);

	return true;
}

bool UAdaGameplayStateComponent::RemoveStateTag(const FGameplayTag StateTag)
{
	if (!ActiveStates.HasMatchingGameplayTag(StateTag))
	{
		return false;
	}
	
	return ActiveStates.UpdateTagCount(StateTag, -1);;
}

FAdaAttribute* UAdaGameplayStateComponent::FindAttribute_Internal(const FGameplayTag AttributeTag)
{
	for (auto It = Attributes.CreateIterator(); It; ++It)
	{
		FAdaAttribute& Attribute = *It;
		if (Attribute.AttributeTag == AttributeTag)
		{
			return &Attribute;
		}
	}

	return nullptr;
}

const FAdaAttribute* UAdaGameplayStateComponent::FindAttribute_Internal(const FGameplayTag AttributeTag) const
{
	for (auto It = Attributes.CreateConstIterator(); It; ++It)
	{
		const FAdaAttribute& Attribute = *It;
		if (Attribute.AttributeTag == AttributeTag)
		{
			return &Attribute;
		}
	}

	return nullptr;
}

FAdaAttribute* UAdaGameplayStateComponent::FindAttributeByIndex(int32 Index)
{
	return Attributes.IsValidIndex(Index) ? &Attributes[Index] : nullptr;
}

const FAdaAttribute* UAdaGameplayStateComponent::FindAttributeByIndex(int32 Index) const
{
	return Attributes.IsValidIndex(Index) ? &Attributes[Index] : nullptr;
}

FAdaAttributeModifier* UAdaGameplayStateComponent::FindModifierByIndex(int32 Index)
{
	return ActiveModifiers.IsValidIndex(Index) ? &ActiveModifiers[Index] : nullptr;
}

const FAdaAttributeModifier* UAdaGameplayStateComponent::FindModifierByIndex(int32 Index) const
{
	return ActiveModifiers.IsValidIndex(Index) ? &ActiveModifiers[Index] : nullptr;
}

bool UAdaGameplayStateComponent::RemoveModifierByIndex(int32 Index)
{
	FAdaAttributeModifier* Modifier = FindModifierByIndex(Index);
	if (!Modifier)
	{
		return false;
	}

	return RemoveModifier_Internal(*Modifier, Index);
}

bool UAdaGameplayStateComponent::RemoveModifier_Internal(FAdaAttributeModifier& Modifier, int32 Index)
{
	FAdaAttribute* Attribute = FindAttribute_Internal(Modifier.AffectedAttribute);
	A_ENSURE_RET(Attribute, false);

	for (uint32 i = Attribute->ActiveModifiers.Num() - 1; i == 0; i--)
	{
		FAdaAttributeModifierHandle& ModifierHandle = Attribute->ActiveModifiers[i];
		if (A_ENSURE(ModifierHandle.Identifier != INDEX_NONE))
		{
			continue;
		}

		if (ModifierHandle.Identifier == Modifier.Identifier)
		{
			Attribute->ActiveModifiers.RemoveAt(i);
		}
	}

	if (Attribute->bIsOverridden && Attribute->OverridingModifier.IsValid() && Attribute->OverridingModifier.Identifier == Modifier.GetIdentifier())
	{
		Attribute->OverridingModifier.Invalidate();
		Attribute->bIsOverridden = false;
	}

	if (Modifier.CalculationType == EAdaAttributeModCalcType::SetByAttribute)
	{
		FAdaAttribute* ModifyingAttribute = FindAttribute_Internal(Modifier.ModifyingAttribute);
		if (A_ENSURE(ModifyingAttribute))
		{
			ModifyingAttribute->AttributeDependencies.Remove(Modifier.AffectedAttribute);
		}
		else
		{
			UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Failed to get modifying attribute %s to attribute %s on modifier removal."), __FUNCTION__, *Modifier.ModifyingAttribute.ToString(), *Modifier.AffectedAttribute.ToString());
		}
	}

	if (Modifier.ParentStatusEffect.IsValid())
	{
		int32 FoundHandleIndex = INDEX_NONE;
		for (int32 ModifierIndex = 0; ModifierIndex < Modifier.ParentStatusEffect->ActiveModifierHandles.Num(); ModifierIndex++)
		{
			FAdaAttributeModifierHandle& Handle = Modifier.ParentStatusEffect->ActiveModifierHandles[ModifierIndex];
			if (Handle.Identifier == Modifier.Identifier)
			{
				FoundHandleIndex = ModifierIndex;
				break;
			}
		}

		if (FoundHandleIndex != INDEX_NONE)
		{
			Modifier.ParentStatusEffect->ActiveModifierHandles.RemoveAt(FoundHandleIndex);
		}
	}

	Attribute->bIsDirty = true;

	ActiveModifiers.RemoveAt(Index);

	return true;
}

void UAdaGameplayStateComponent::ApplyImmediateModifier(FAdaAttribute& Attribute, const FAdaAttributeModifierSpec& ModifierToApply)
{
	// Get base value.
	float BaseValue = Attribute.BaseValue;

	if (ModifierToApply.ModifiesClamping())
	{
		A_ENSURE_MSG_RET(Attribute.bUsesClamping, void(), TEXT("Tried to clamp unclamped attribute %s!"), *Attribute.AttributeTag.ToString());
		
		Attribute.BaseClampingValues.X += ModifierToApply.ClampingParams.bHasMinDelta ? ModifierToApply.ClampingParams.MinDelta : 0.0f;
		Attribute.BaseClampingValues.Y += ModifierToApply.ClampingParams.bHasMaxDelta ? ModifierToApply.ClampingParams.MaxDelta : 0.0f;
	}

	if (ModifierToApply.OperationType == EAdaAttributeModOpType::Additive ||ModifierToApply.OperationType == EAdaAttributeModOpType::PostAdditive)
	{
		BaseValue += ModifierToApply.ModifierValue;
	}
	else if (ModifierToApply.OperationType == EAdaAttributeModOpType::Override)
	{
		// Treat immediate overrides as setting this attribute.
		BaseValue = ModifierToApply.ModifierValue;
	}

	// Apply base value changes to the current value.
	float CurrentValue = Attribute.CurrentValue + BaseValue - Attribute.BaseValue;

	// Clamp base and current if required.
	if (Attribute.bUsesClamping)
	{
		BaseValue = FMath::Clamp(BaseValue, Attribute.BaseClampingValues.X, Attribute.BaseClampingValues.Y);

		// Recalculate current value to use the clamped base value.
		CurrentValue = Attribute.CurrentValue + BaseValue - Attribute.BaseValue;
		
		CurrentValue = FMath::Clamp(CurrentValue, Attribute.CurrentClampingValues.X, Attribute.CurrentClampingValues.Y);
	}

	const float OldBase = Attribute.BaseValue;
	const float OldCurrent = Attribute.CurrentValue;
	Attribute.BaseValue = BaseValue;
	Attribute.CurrentValue = CurrentValue;

	if (!ModifierToApply.bRecalculateImmediately)
	{
		NotifyAttributeChanged(Attribute, OldBase, OldCurrent);
	}
}

void UAdaGameplayStateComponent::ApplyOverridingModifier(FAdaAttribute& Attribute, const FAdaAttributeModifier& Modifier, const int32 ModifierIndex)
{
	Attribute.bIsOverridden = true;
	Attribute.OverridingModifier = FAdaAttributeModifierHandle(this, ModifierIndex, Modifier.Identifier);

	if (Modifier.ModifiesClamping())
	{
		A_ENSURE_MSG_RET(Attribute.bUsesClamping, void(0), TEXT("Tried to clamp unclamped attribute %s!"), *Attribute.AttributeTag.ToString());
		
		Attribute.CurrentClampingValues.X += Modifier.ClampingParams.bHasMinDelta ? Modifier.ClampingParams.MinDelta : 0.0f;
		Attribute.CurrentClampingValues.Y += Modifier.ClampingParams.bHasMaxDelta ? Modifier.ClampingParams.MaxDelta : 0.0f;
	}
}

void UAdaGameplayStateComponent::RecalculateAttribute(FAdaAttribute& Attribute, const uint64& CurrentTick)
{
	// Get base value.
	float BaseValue = Attribute.BaseValue;
	
	// Reset current value to base value.
	float CurrentValue = Attribute.BaseValue;
	
	bool bWasOverridden = false;
	if (Attribute.bIsOverridden)
	{
		A_ENSURE_RET(Attribute.OverridingModifier.IsValid(), void(0));
		FAdaAttributeModifier* OverridingModifier = FindModifierByIndex(Attribute.OverridingModifier.Index);
		CurrentValue = OverridingModifier->ModifierValue;
		bWasOverridden = true;
	}

	if (!bWasOverridden)
	{
		// Reset clamping values prior to potential recalculation.
		Attribute.CurrentClampingValues = Attribute.BaseClampingValues;
		
		// Calculation formula:
		// ((BaseValue + Additive) * Multiply) + PostAdditive;
		// Then the same for current, using base
		float AggregatedBaseAdditives = 0.0f;
		float AggregatedBaseMultipliers = 1.0f;
		float AggregatedBasePostAdditives = 0.0f;
		
		float AggregatedCurrentAdditives = 0.0f;
		float AggregatedCurrentMultipliers = 1.0f;
		float AggregatedCurrentPostAdditives = 0.0f;
		
		// Aggregate modifiers from the attribute's active modifier list.
		for (FAdaAttributeModifierHandle& ModifierHandle : Attribute.ActiveModifiers)
		{
			FAdaAttributeModifier* Modifier = FindModifierByIndex(ModifierHandle.Index);
			if (!Modifier)
			{
				UE_LOG(LogAdaGameplayState, Warning, TEXT("%hs: Invalid modifier to attribute %s!"), __FUNCTION__, *Attribute.AttributeTag.ToString());
				continue;
			}

			if (!Modifier->CanApply(CurrentTick))
			{
				continue;
			}

			if (Modifier->ModifiesClamping())
			{
				A_ENSURE_MSG_RET(Attribute.bUsesClamping, void(0), TEXT("Tried to clamp unclamped attribute %s!"), *Attribute.AttributeTag.ToString());
		
				Attribute.CurrentClampingValues.X += Modifier->ClampingParams.bHasMinDelta ? Modifier->ClampingParams.MinDelta : 0.0f;
				Attribute.CurrentClampingValues.Y += Modifier->ClampingParams.bHasMaxDelta ? Modifier->ClampingParams.MaxDelta : 0.0f;
			}

			float ModifierValue = Modifier->ModifierValue;
			if (Modifier->bAffectsBase)
			{
				switch (Modifier->OperationType)
				{
					case EAdaAttributeModOpType::Additive:
					{
						AggregatedBaseAdditives += ModifierValue;
						break;
					}
					case EAdaAttributeModOpType::Multiply:
					{
						AggregatedBaseMultipliers *= ModifierValue;
						break;
					}
					case EAdaAttributeModOpType::PostAdditive:
					{
						AggregatedBasePostAdditives += ModifierValue;
						break;
					}
					default: break;
				}
			}
			else
			{
				switch (Modifier->OperationType)
				{
					case EAdaAttributeModOpType::Additive:
					{
						AggregatedCurrentAdditives += ModifierValue;
						break;
					}
					case EAdaAttributeModOpType::Multiply:
					{
						AggregatedCurrentMultipliers *= ModifierValue;
						break;
					}
					case EAdaAttributeModOpType::PostAdditive:
					{
						AggregatedCurrentPostAdditives += ModifierValue;
						break;
					}
					default: break;
				}
			}

			Modifier->PostApply(CurrentTick);
		}

		BaseValue = ((BaseValue + AggregatedBaseAdditives) * AggregatedBaseMultipliers) + AggregatedBasePostAdditives;
		CurrentValue = ((BaseValue + AggregatedCurrentAdditives) * AggregatedCurrentMultipliers) + AggregatedCurrentPostAdditives;
	}

	// Clamp base and current if required.
	if (Attribute.bUsesClamping)
	{
		BaseValue = FMath::Clamp(BaseValue, Attribute.BaseClampingValues.X, Attribute.BaseClampingValues.Y);
		CurrentValue = FMath::Clamp(CurrentValue, Attribute.CurrentClampingValues.X, Attribute.CurrentClampingValues.Y);
	}

	const float OldBase = Attribute.BaseValue;
	const float OldCurrent = Attribute.CurrentValue;
	Attribute.BaseValue = BaseValue;
	Attribute.CurrentValue = CurrentValue;

	NotifyAttributeChanged(Attribute, OldBase, OldCurrent);

	// We've finished recalculating, so this attribute is no longer dirty.
	Attribute.bIsDirty = false;
}

bool UAdaGameplayStateComponent::DoesAttributeDependOnOther(const FGameplayTag AttributeTag, const FGameplayTag OtherAttributeTag) const
{
	const FAdaAttribute* const Attribute = FindAttribute_Internal(AttributeTag);
	if (!Attribute)
	{
		UE_LOG(LogAdaGameplayState, Warning, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
		return false;
	}

	const FAdaAttribute* const OtherAttribute = FindAttribute_Internal(OtherAttributeTag);
	if (!OtherAttribute)
	{
		UE_LOG(LogAdaGameplayState, Warning, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *OtherAttributeTag.ToString(), *GetNameSafe(this));
		return false;
	}

	return Attribute->AttributeDependencies.Contains(OtherAttributeTag);
}

void UAdaGameplayStateComponent::NotifyAttributeChanged(FAdaAttribute& Attribute, const float OldBase, const float OldCurrent)
{
	// Value didn't change, so early return.
	if (!FMath::IsNearlyEqual(Attribute.BaseValue, OldBase, 1E-04) || !FMath::IsNearlyEqual(Attribute.CurrentValue, OldCurrent, 1E-04))
	{
		return;
	}
	
	// Update any modifiers that use this attribute and set those attributes as dirty for recalculation.
	for (auto& [DependentAttributeTag, Index]: Attribute.AttributeDependencies)
	{
		FAdaAttribute* DependentAttribute = FindAttribute_Internal(DependentAttributeTag);
		if (!DependentAttribute)
		{
			UE_LOG(LogAdaGameplayState, Warning, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *DependentAttributeTag.ToString(), *GetNameSafe(this));
			continue;
		}

		// Find the modifier that uses this attribute and update the value.
		FAdaAttributeModifier* Modifier = FindModifierByIndex(Index);
		if (!Modifier)
		{
			continue;
		}

		if (!A_ENSURE(Modifier->CalculationType == EAdaAttributeModCalcType::SetByAttribute))
		{
			continue;
		}
		
		Modifier->SetValue(Attribute.CurrentValue);
		DependentAttribute->bIsDirty = true;
	}

	if (Attribute.OnAttributeUpdated.IsBound())
	{
		Attribute.OnAttributeUpdated.Broadcast(Attribute.AttributeTag, Attribute.BaseValue, Attribute.CurrentValue, OldBase, OldCurrent);
	}
}

int32 UAdaGameplayStateComponent::GetNextAttributeId()
{
	LatestModifierId++;
	if (LatestModifierId == INDEX_NONE)
	{
		LatestModifierId = 0;
	}

	return LatestModifierId;
}

int32 UAdaGameplayStateComponent::GetNextModifierId()
{
	LatestModifierId++;
	if (LatestModifierId == INDEX_NONE)
	{
		LatestModifierId = 0;
	}

	return LatestModifierId;
}

int32 UAdaGameplayStateComponent::GetNextStatusEffectId()
{
	LatestStatusEffectId++;
	if (LatestStatusEffectId == INDEX_NONE)
	{
		LatestStatusEffectId = 0;
	}

	return LatestStatusEffectId;
}

bool UAdaGameplayStateComponent::RemoveStatusEffect_Internal(const int32 Index)
{
	if (!ActiveStatusEffects.IsValidIndex(Index))
	{
		return false;
	}
	
	UAdaStatusEffect* StatusEffect = ActiveStatusEffects[Index].Get();
	A_ENSURE_RET(IsValid(StatusEffect), false);

	const FGameplayTag StatusEffectTag = StatusEffect->GetEffectTag();

	const UWorld* const World = GetWorld();
	A_ENSURE_RET(IsValid(World), false);

	const AAdaGameState* const GameState = World->GetGameState<AAdaGameState>();
	A_ENSURE_RET(IsValid(GameState), false);

	const UAdaGameplayStateManager* const StateManager = GameState->GetGameplayStateManager();
	A_ENSURE_RET(IsValid(StateManager), false);

	const UAdaStatusEffectDefinition* const StatusEffectDef = StateManager->GetStatusEffectDefinition(StatusEffectTag);
	A_ENSURE_RET(IsValid(StatusEffectDef), false);
	
	ActiveStates.UpdateTagCount(StatusEffectDef->StateTagsToAdd, -1);
	ActiveStatusEffectTags.UpdateTagCount(StatusEffectTag, -1);

	// Reverse iteration as we'll be modifying the active modifier handles array as we remove modifiers.
	for (uint32 i = StatusEffect->ActiveModifierHandles.Num() - 1; i == 0; i--)
	{
		RemoveModifier(StatusEffect->ActiveModifierHandles[i]);
	}
	
	ActiveStatusEffects.RemoveAt(Index);

	return true;
}