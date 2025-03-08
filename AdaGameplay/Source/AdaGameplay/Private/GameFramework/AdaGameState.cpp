// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameFramework/AdaGameState.h"

#include "Debug/AdaAssertionMacros.h"
#include "GameplayState/AdaGameplayStateManager.h"

AAdaGameState::AAdaGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayStateManager = CreateDefaultSubobject<UAdaGameplayStateManager>("GameplayStateManager");
	A_ENSURE(IsValid(GameplayStateManager));
}
