// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "AdaStatusEffectTypes.generated.h"

class UAdaStatusEffectDefinition;
class UAdaGameplayStateComponent;

USTRUCT()
struct ADAGAMEPLAY_API FAdaStatusEffectHandle
{
	GENERATED_BODY()

	friend class UAdaGameplayStateComponent;

public:
	FAdaStatusEffectHandle() = default;
	FAdaStatusEffectHandle(UAdaGameplayStateComponent* Owner, const int32 InIndex, const int32 InId);
	
	bool IsValid() const;
	void Invalidate();

	const UAdaStatusEffectDefinition* Get() const;

	bool Remove();
	
private:
	// The component that owns this status effect.
	TWeakObjectPtr<UAdaGameplayStateComponent> OwningStateComponentWeak = nullptr;

	// The index in the owning component's array for this status effect, when it's valid.
	int32 Index = INDEX_NONE;

	// The identifier assigned to this status effect.
	int32 Identifier = INDEX_NONE;
};