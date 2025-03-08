// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaGameplayStateComponent.h"

#include "GameFramework/AdaGameState.h"
#include "GameplayState/AdaGameplayStateManager.h"
#include "Debug/AdaAssertionMacros.h"

DEFINE_LOG_CATEGORY(LogAdaGameplayState);

UAdaGameplayStateComponent::UAdaGameplayStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAdaGameplayStateComponent::BeginPlay()
{
	Super::BeginPlay();

	const UWorld* const World = GetWorld();
	A_VALIDATE_OBJ(World, void(0));

	const AAdaGameState* const GameState = World->GetGameState<AAdaGameState>();
	A_VALIDATE_OBJ(GameState, void(0));

	UAdaGameplayStateManager* StateManager = GameState->GetGameplayStateManager();
	A_VALIDATE_OBJ(StateManager, void(0));

	StateManager->RegisterStateComponent(this);
}

void UAdaGameplayStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ON_SCOPE_EXIT
	{
		Super::EndPlay(EndPlayReason);
	};

	const UWorld* const World = GetWorld();
	A_VALIDATE_OBJ(World, void(0));

	const AAdaGameState* const GameState = World->GetGameState<AAdaGameState>();
	A_VALIDATE_OBJ(GameState, void(0));

	UAdaGameplayStateManager* StateManager = GameState->GetGameplayStateManager();
	A_VALIDATE_OBJ(StateManager, void(0));

	StateManager->UnregisterStateComponent(this);
}

void UAdaGameplayStateComponent::FixedTick(const uint64& CurrentFrame)
{
	const UWorld* const World = GetWorld();
	A_VALIDATE_OBJ(World, void(0));

	TArray<TPair<TSharedRef<FAdaAttributeModifier>, uint32>> ExpiredModifiers;
	TArray<TPair<TSharedRef<FAdaAttributeModifier>, uint32>> PostTick_ExpiredModifiers;

	// Maintain internal frame reference.
	LatestFrame = CurrentFrame;
	
	for (auto It = ActiveModifiers.CreateIterator(); It; ++It)
	{
		int32 Index = It.GetIndex();
		
		TSharedRef<FAdaAttributeModifier> ModifierRef = *It;
		FAdaAttributeModifier& Modifier = ModifierRef.Get();

		if (Modifier.ApplicationType != EAdaAttributeModApplicationType::Persistent)
		{
			if (!Modifier.CanApply(CurrentFrame))
			{
				continue;
			}

			TSharedPtr<FAdaAttribute> FoundAttribute = FindAttribute_Internal(Modifier.AffectedAttribute);
			if (!FoundAttribute.IsValid())
			{
				UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Invalid periodic modifier for attribute %s"), __FUNCTION__, *Modifier.AffectedAttribute.ToString());
				ExpiredModifiers.Add({ModifierRef, Index});
				continue;
			}

			bool bTryRecalculate = true;
			if (Modifier.HasExpired(CurrentFrame))
			{
				if (Modifier.bShouldApplyOnRemoval)
				{
					PostTick_ExpiredModifiers.Add({ModifierRef, Index});
				}
				else
				{
					ExpiredModifiers.Add({ModifierRef, Index});
					bTryRecalculate = false;
				}
			}

			// Update dynamic modifiers.
			if (bTryRecalculate && Modifier.ShouldRecalculate())
			{
				Modifier.CalculateValue();
			}

			FAdaAttribute& Attribute = *FoundAttribute;
			Attribute.bIsDirty = true;
		}
	}

	for (auto& [ExpiredModifier, Index] : ExpiredModifiers)
	{
		RemoveModifier_Internal(ExpiredModifier, Index);
	}

	// Update attributes.
	for (TSharedRef<FAdaAttribute>& AttributeRef : Attributes)
	{
		FAdaAttribute& Attribute = AttributeRef.Get();
		if (Attribute.bIsDirty)
		{
			RecalculateAttribute(Attribute, CurrentFrame);
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

	const TSharedRef<FAdaAttribute>& NewAttribute = Attributes.Add_GetRef(MakeShareable<FAdaAttribute>(new FAdaAttribute(AttributeTag, InitParams)));

	return MakeAttributeHandle(NewAttribute);
}

void UAdaGameplayStateComponent::RemoveAttribute(const FGameplayTag AttributeTag)
{
	TOptional<TSharedRef<FAdaAttribute>> FoundAttributeOptional = FindAttributeRef_Internal(AttributeTag);
	if (!FoundAttributeOptional.IsSet())
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
		return;
	}

	const TSharedRef<FAdaAttribute>& FoundAttribute = FoundAttributeOptional.GetValue();
	for (auto& [DependentAttributeTag, Index]: FoundAttribute->AttributeDependencies)
	{
		RemoveModifierByIndex(Index);
	}
	
	Attributes.Remove(FoundAttribute);
}

FAdaAttributeHandle UAdaGameplayStateComponent::FindAttribute(const FGameplayTag AttributeTag)
{
	for (TSharedRef<FAdaAttribute> Attribute : Attributes)
	{
		if (Attribute.Get().AttributeTag == AttributeTag)
		{
			return MakeAttributeHandle(Attribute);
		}
	}

	UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
	return FAdaAttributeHandle();
}

void UAdaGameplayStateComponent::InvalidateHandle(FAdaAttributeHandle& InHandle)
{
	if (!InHandle.IsValid())
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Tried to invalidate invalid attribute handle!"), __FUNCTION__);
		return;
	}
	
	if (FindAttribute_Internal(InHandle.AttributeTag))
	{
		InHandle.Invalidate();
		return;
	}

	UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *InHandle.AttributeTag.ToString(), *GetNameSafe(this));
}

bool UAdaGameplayStateComponent::HasAttribute(const FGameplayTag AttributeTag) const
{
	if (FindAttribute_Internal(AttributeTag))
	{
		return true;
	}
	
	return false;
}

FAdaOnAttributeUpdated* UAdaGameplayStateComponent::GetDelegateForAttribute(const FGameplayTag AttributeTag)
{
	TSharedPtr<FAdaAttribute> Attribute = FindAttribute_Internal(AttributeTag);
	if (!Attribute.IsValid())
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
		return nullptr;
	}

	return &Attribute->OnAttributeUpdated;
}

FAdaAttributeModifierHandle UAdaGameplayStateComponent::ModifyAttribute(const FGameplayTag AttributeTag, const FAdaAttributeModifierSpec& ModifierToApply)
{
	FAdaAttributeModifierHandle OutHandle = FAdaAttributeModifierHandle();
	
	TSharedPtr<FAdaAttribute> FoundAttribute = FindAttribute_Internal(AttributeTag);
	if (!FoundAttribute.IsValid())
	{
		UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
		return OutHandle;
	}

	// Can't modify attributes that are currently overridden unless the override is removed first.
	if (FoundAttribute->bIsOverridden)
	{
		return OutHandle;
	}

	if (!IsModifierValid(ModifierToApply))
	{
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
		// Create the modifier, but don't cache it yet as we may have extra setup to do first.
		const TSharedRef<FAdaAttributeModifier>& ModifierRef = MakeShareable<FAdaAttributeModifier>(new FAdaAttributeModifier(AttributeTag, ModifierToApply, LatestFrame));
		
		FAdaAttributeModifier& Modifier = ModifierRef.Get();
		int32 OutIndex = INDEX_NONE;
		if (Modifier.CalculationType == EAdaAttributeModCalcType::SetByAttribute)
		{
			TSharedPtr<FAdaAttribute> ModifyingAttribute = FindAttribute_Internal(ModifierToApply.ModifyingAttribute);
			if (!ModifyingAttribute.IsValid())
			{
				UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *ModifierToApply.ModifyingAttribute.ToString(), *GetNameSafe(this));
				return OutHandle;
			}

			if (DoesAttributeDependOnOther(ModifierToApply.ModifyingAttribute, AttributeTag))
			{
				UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Attempted to create circular modifier dependency for attributes %s and %s!"), __FUNCTION__, *AttributeTag.ToString(), *ModifierToApply.ModifyingAttribute.ToString());
				return OutHandle;
			}

			// Cache the modifier.
			OutIndex = ActiveModifiers.Add(ModifierRef);
			Attribute.ActiveModifiers.Add(ModifierRef);
			
			Modifier.SetModifyingAttribute(*ModifyingAttribute);
			ModifyingAttribute->AttributeDependencies.Add({AttributeTag, OutIndex});
		}
		else
		{
			// Cache the modifier.
			OutIndex = ActiveModifiers.Add(ModifierRef);
			Attribute.ActiveModifiers.Add(ModifierRef);
		}

		if (ModifierToApply.OperationType == EAdaAttributeModOpType::Override)
		{
			ApplyOverridingModifier(Attribute, ModifierRef);
		}

		A_ENSURE(OutIndex != INDEX_NONE);
		OutHandle = FAdaAttributeModifierHandle(this, ModifierToApply.ApplicationType, OutIndex);
	}

	if (ModifierToApply.bRecalculateImmediately)
	{
		RecalculateAttribute(Attribute, LatestFrame);
	}
	else
	{
		// #TODO: Figure out if it's actually safer to just recalculate immediately.
		Attribute.bIsDirty = true;
	}

	return OutHandle;
}

bool UAdaGameplayStateComponent::RemoveModifier(FAdaAttributeModifierHandle& ModifierHandle)
{
	TOptional<TSharedRef<FAdaAttributeModifier>> ModifierOptional = FindModifier(ModifierHandle.Index);
	if (!ModifierOptional.IsSet())
	{
		return false;
	}

	TSharedRef<FAdaAttributeModifier>& Modifier = ModifierOptional.GetValue();

	RemoveModifier_Internal(Modifier, ModifierHandle.Index);
	ModifierHandle.Invalidate();
	
	return true;
}

TSharedPtr<FAdaAttribute> UAdaGameplayStateComponent::FindAttribute_Internal(const FGameplayTag AttributeTag)
{
	for (TSharedRef<FAdaAttribute>& Attribute : Attributes)
	{
		if (Attribute.Get().AttributeTag == AttributeTag)
		{
			return Attribute.ToSharedPtr();
		}
	}

	return nullptr;
}

const TSharedPtr<FAdaAttribute> UAdaGameplayStateComponent::FindAttribute_Internal(const FGameplayTag AttributeTag) const
{
	for (const TSharedRef<FAdaAttribute>& Attribute : Attributes)
	{
		if (Attribute.Get().AttributeTag == AttributeTag)
		{
			return Attribute.ToSharedPtr();
		}
	}

	return nullptr;
}

TOptional<TSharedRef<FAdaAttribute>> UAdaGameplayStateComponent::FindAttributeRef_Internal(const FGameplayTag AttributeTag)
{
	for (const TSharedRef<FAdaAttribute>& Attribute : Attributes)
	{
		if (Attribute.Get().AttributeTag == AttributeTag)
		{
			return Attribute;
		}
	}

	return TOptional<TSharedRef<FAdaAttribute>>();
}

TOptional<TSharedRef<FAdaAttributeModifier>> UAdaGameplayStateComponent::FindModifier(int32 Index)
{
	if (!ActiveModifiers.IsValidIndex(Index))
	{
		return TOptional<TSharedRef<FAdaAttributeModifier>>();
	}
	
	// #TODO: Do some error checking in here to make sure we're actually fetching the right modifier.
	return ActiveModifiers[Index];
}

const TOptional<TSharedRef<FAdaAttributeModifier>> UAdaGameplayStateComponent::FindModifier(int32 Index) const
{
	if (!ActiveModifiers.IsValidIndex(Index))
	{
		return TOptional<TSharedRef<FAdaAttributeModifier>>();
	}
	
	// #TODO: Do some error checking in here to make sure we're actually fetching the right modifier.
	return ActiveModifiers[Index];
}

bool UAdaGameplayStateComponent::RemoveModifierByIndex(int32 Index)
{
	TOptional<TSharedRef<FAdaAttributeModifier>> ModifierOptional = FindModifier(Index);
	if (!ModifierOptional.IsSet())
	{
		return false;
	}

	TSharedRef<FAdaAttributeModifier>& Modifier = ModifierOptional.GetValue();

	return RemoveModifier_Internal(Modifier, Index);
}

bool UAdaGameplayStateComponent::RemoveModifier_Internal(TSharedRef<FAdaAttributeModifier>& Modifier, int32 Index)
{
	TSharedPtr<FAdaAttribute> Attribute = FindAttribute_Internal(Modifier->AffectedAttribute);
	if (!A_ENSURE(Attribute.IsValid()))
	{
		return false;
	}

	Attribute->ActiveModifiers.Remove(Modifier);

	if (Attribute->bIsOverridden && Attribute->OverridingModifier == Modifier)
	{
		Attribute->OverridingModifier = nullptr;
		Attribute->bIsOverridden = false;
	}

	if (Modifier->CalculationType == EAdaAttributeModCalcType::SetByAttribute)
	{
		TSharedPtr<FAdaAttribute> ModifyingAttribute = FindAttribute_Internal(Modifier->ModifyingAttribute);
		if (A_ENSURE(ModifyingAttribute.IsValid()))
		{
			ModifyingAttribute->AttributeDependencies.Remove(Modifier->AffectedAttribute);
		}
		else
		{
			UE_LOG(LogAdaGameplayState, Error, TEXT("%hs: Failed to get modifying attribute %s to attribute %s on modifier removal."), __FUNCTION__, *Modifier->ModifyingAttribute.ToString(), *Modifier->AffectedAttribute.ToString());
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
		BaseValue = FMath::Clamp(BaseValue, Attribute.MinValue, Attribute.MaxValue);

		// Recalculate current value to use the clamped base value.
		CurrentValue = Attribute.CurrentValue + BaseValue - Attribute.BaseValue;
		
		CurrentValue = FMath::Clamp(CurrentValue, Attribute.MinValue, Attribute.MaxValue);
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

void UAdaGameplayStateComponent::ApplyOverridingModifier(FAdaAttribute& Attribute, const TSharedRef<FAdaAttributeModifier>& Modifier)
{
	Attribute.bIsOverridden = true;
	Attribute.OverridingModifier = Modifier;
}

void UAdaGameplayStateComponent::RecalculateAttribute(FAdaAttribute& Attribute, const uint64& CurrentFrame)
{
	// Get base value.
	float BaseValue = Attribute.BaseValue;
	
	// Reset current value to base value.
	float CurrentValue = Attribute.BaseValue;
	
	bool bWasOverridden = false;
	if (Attribute.bIsOverridden)
	{
		A_ENSURE_RET(Attribute.OverridingModifier.IsValid(), void(0));
		TSharedPtr<FAdaAttributeModifier> OverridingModifier = Attribute.OverridingModifier.Pin();
		CurrentValue = OverridingModifier->ModifierValue;
		bWasOverridden = true;
	}

	if (!bWasOverridden)
	{
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
		for (TWeakPtr<FAdaAttributeModifier> ModifierWeak : Attribute.ActiveModifiers)
		{
			TSharedPtr<FAdaAttributeModifier> ModifierPtr = ModifierWeak.Pin();
			if (!ModifierPtr.IsValid())
			{
				UE_LOG(LogAdaGameplayState, Warning, TEXT("%hs: Invalid modifier to attribute %s!"), __FUNCTION__, *Attribute.AttributeTag.ToString());
				continue;
			}

			FAdaAttributeModifier* Modifier = ModifierPtr.Get();
			if (!Modifier)
			{
				UE_LOG(LogAdaGameplayState, Warning, TEXT("%hs: Invalid modifier to attribute %s!"), __FUNCTION__, *Attribute.AttributeTag.ToString());
				continue;
			}

			if (!Modifier->CanApply(CurrentFrame))
			{
				continue;
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
						AggregatedBaseMultipliers += ModifierValue;
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
						AggregatedCurrentMultipliers += ModifierValue;
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

			Modifier->PostApply(CurrentFrame);
		}

		BaseValue = ((BaseValue + AggregatedBaseAdditives) * AggregatedBaseMultipliers) + AggregatedBasePostAdditives;
		CurrentValue = ((BaseValue + AggregatedCurrentAdditives) * AggregatedCurrentMultipliers) + AggregatedCurrentPostAdditives;
	}

	// Clamp base and current if required.
	if (Attribute.bUsesClamping)
	{
		BaseValue = FMath::Clamp(BaseValue, Attribute.MinValue, Attribute.MaxValue);
		CurrentValue = FMath::Clamp(CurrentValue, Attribute.MinValue, Attribute.MaxValue);
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
	TSharedPtr<FAdaAttribute> Attribute = FindAttribute_Internal(AttributeTag);
	if (!Attribute.IsValid())
	{
		UE_LOG(LogAdaGameplayState, Warning, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *AttributeTag.ToString(), *GetNameSafe(this));
		return false;
	}

	TSharedPtr<FAdaAttribute> OtherAttribute = FindAttribute_Internal(OtherAttributeTag);
	if (!OtherAttribute.IsValid())
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
		TSharedPtr<FAdaAttribute> DependentAttribute = FindAttribute_Internal(DependentAttributeTag);
		if (!DependentAttribute.IsValid())
		{
			UE_LOG(LogAdaGameplayState, Warning, TEXT("%hs: Unable to find attribute %s on component %s"), __FUNCTION__, *DependentAttributeTag.ToString(), *GetNameSafe(this));
			continue;
		}

		// Find the modifier that uses this attribute and update the value.
		TOptional<TSharedRef<FAdaAttributeModifier>> ModifierOptional = FindModifier(Index);
		if (!ModifierOptional.IsSet())
		{
			continue;
		}

		TSharedRef<FAdaAttributeModifier>& Modifier = ModifierOptional.GetValue();
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

bool UAdaGameplayStateComponent::IsModifierValid(const FAdaAttributeModifierSpec& Modifier)
{
	switch (Modifier.ApplicationType)
	{
		case EAdaAttributeModApplicationType::Instant:
		{
			return Modifier.bAffectsBase && Modifier.OperationType != EAdaAttributeModOpType::Multiply;
		}
		case EAdaAttributeModApplicationType::Duration:
		{
			return !Modifier.bAffectsBase;
		}
		case EAdaAttributeModApplicationType::Periodic:
		{
			return Modifier.bAffectsBase && Modifier.OperationType != EAdaAttributeModOpType::Multiply && Modifier.OperationType != EAdaAttributeModOpType::Override;
		}
		case EAdaAttributeModApplicationType::Ticking:
		{
			return Modifier.bAffectsBase && Modifier.OperationType != EAdaAttributeModOpType::Multiply && Modifier.OperationType != EAdaAttributeModOpType::Override;
		}
		case EAdaAttributeModApplicationType::Persistent:
		{
			return !Modifier.bAffectsBase;
		}
		default: break;
	}

	return false;
}

FAdaAttributeHandle UAdaGameplayStateComponent::MakeAttributeHandle(const TSharedRef<FAdaAttribute>& InAttribute)
{
	return FAdaAttributeHandle(InAttribute);
}