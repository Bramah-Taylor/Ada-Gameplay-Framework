// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameFramework/GameStateBase.h"

#include "AdaGameState.generated.h"

class UAdaGameplayStateManager;

UCLASS()
class ADAGAMEPLAY_API AAdaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AAdaGameState(const FObjectInitializer& ObjectInitializer);

	inline UAdaGameplayStateManager* GetGameplayStateManager() const {return GameplayStateManager; };

protected:
	UPROPERTY()
	TObjectPtr<UAdaGameplayStateManager> GameplayStateManager = nullptr;
};
