// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameFramework/AdaGameState.h"

#include "Debug/AdaAssertionMacros.h"
#include "GameplayState/AdaGameplayStateManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AdaGameState)

AAdaGameState::AAdaGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayStateManager = CreateDefaultSubobject<UAdaGameplayStateManager>("GameplayStateManager");
	A_ENSURE(IsValid(GameplayStateManager));
}
