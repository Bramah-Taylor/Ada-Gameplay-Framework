// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaStatusEffectTypes.h"

#include "GameplayState/AdaGameplayStateComponent.h"

FAdaStatusEffectHandle::FAdaStatusEffectHandle(UAdaGameplayStateComponent* Owner, const int32 InIndex, const int32 InId) :
	OwningStateComponentWeak(Owner),
	Index(InIndex),
	Identifier(InId)
{
	
}

bool FAdaStatusEffectHandle::IsValid() const
{
	// #TODO(Ada.Gameplay): Implement
	return false;
}

void FAdaStatusEffectHandle::Invalidate()
{
	OwningStateComponentWeak = nullptr;
	Index = INDEX_NONE;
	Identifier = INDEX_NONE;
}

const UAdaStatusEffectDefinition* FAdaStatusEffectHandle::Get() const
{
	// #TODO(Ada.Gameplay): Implement
	return nullptr;
}

bool FAdaStatusEffectHandle::Remove()
{
	// #TODO(Ada.Gameplay): Implement
	return false;
}