// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaStatusEffectTypes.h"

#include "Debug/AdaAssertionMacros.h"
#include "GameplayState/AdaGameplayStateComponent.h"
#include "GameplayState/AdaStatusEffect.h"

FAdaStatusEffectHandle::FAdaStatusEffectHandle(UAdaGameplayStateComponent* Owner, const int32 InIndex, const int32 InId) :
	OwningStateComponentWeak(Owner),
	Index(InIndex),
	Identifier(InId)
{
	
}

bool FAdaStatusEffectHandle::IsValid(bool bValidateOwner) const
{
	if (Identifier == INDEX_NONE || Index == INDEX_NONE)
	{
		return false;
	}

	if (!bValidateOwner)
	{
		return true;
	}
	
	UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, false);

	if (OwningStateComponent->FindStatusEffect(*this))
	{
		return true;
	}

	return false;
}

void FAdaStatusEffectHandle::Invalidate()
{
	OwningStateComponentWeak = nullptr;
	Index = INDEX_NONE;
	Identifier = INDEX_NONE;
}

const UAdaStatusEffect* FAdaStatusEffectHandle::Get() const
{
	if (Identifier == INDEX_NONE || Index == INDEX_NONE)
	{
		return nullptr;
	}
	
	const UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, nullptr);

	return OwningStateComponent->FindStatusEffect(*this);
}

bool FAdaStatusEffectHandle::Remove()
{
	if (Identifier == INDEX_NONE || Index == INDEX_NONE)
	{
		return false;
	}
	
	UAdaGameplayStateComponent* const OwningStateComponent = OwningStateComponentWeak.Get();
	A_VALIDATE_OBJ(OwningStateComponent, false);

	return OwningStateComponent->RemoveStatusEffect(*this);
}