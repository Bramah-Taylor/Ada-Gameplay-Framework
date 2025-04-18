// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"

#include "AdaGameplayStateManager.generated.h"

class UAdaGameplayStateComponent;
class UAdaStatusEffectDefinition;

#define ADA_TICK_BUCKET_COUNT 15

UCLASS()
class ADAGAMEPLAY_API UAdaGameplayStateManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAdaGameplayStateManager();

	void RegisterStateComponent(UAdaGameplayStateComponent* StateComponent);
	void UnregisterStateComponent(UAdaGameplayStateComponent* StateComponent);

	const UAdaStatusEffectDefinition* GetStatusEffectDefinition(const FGameplayTag EffectTag) const;

	static const UCurveFloat* GetCurveForModifier(const FGameplayTag CurveTag);

protected:
	// Begin UActorComponent overrides
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// End UActorComponent overrides.

	void FixedTick(const uint64& CurrentFrame);

	void IncrementAssignmentCounter();
	void IncrementTickCounter();

private:
	struct FTickBucket
	{
		TArray<TWeakObjectPtr<UAdaGameplayStateComponent>> Components;
		uint64 CurrentFrame = 0;
	};
	
	TArray<FTickBucket> TickBuckets;
	TMap<FObjectKey, uint8> ComponentToBucketMap;

	uint8 NextBucketToTick = 0;
	uint8 NextBucketToAssign = 0;

	TMap<FGameplayTag, TObjectPtr<const UAdaStatusEffectDefinition>> LoadedStatusEffectDefinitions;
};
