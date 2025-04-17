// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaGameplayStateManager.h"

#include "DataRegistrySubsystem.h"

#include "Simulation/AdaTickManager.h"
#include "GameplayState/AdaGameplayStateComponent.h"
#include "Debug/AdaAssertionMacros.h"

UAdaGameplayStateManager::UAdaGameplayStateManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UAdaGameplayStateManager::InitializeComponent()
{
	Super::InitializeComponent();
	
	TickBuckets.AddDefaulted(ADA_TICK_BUCKET_COUNT);
}

void UAdaGameplayStateManager::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	A_ENSURE_RET(IsValid(World), void(0));

	UAdaTickManager* TickManager = World->GetSubsystem<UAdaTickManager>();
	A_ENSURE_RET(IsValid(TickManager), void(0));

	TickManager->RegisterTickFunction(this, &UAdaGameplayStateManager::FixedTick);
}

void UAdaGameplayStateManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UWorld* World = GetWorld();
	A_VALIDATE_OBJ(World, void(0));

	UAdaTickManager* TickManager = World->GetSubsystem<UAdaTickManager>();
	A_VALIDATE_OBJ(TickManager, void(0));

	TickManager->UnregisterTickFunction(this);
	
	Super::EndPlay(EndPlayReason);
}

void UAdaGameplayStateManager::RegisterStateComponent(UAdaGameplayStateComponent* StateComponent)
{
	A_VALIDATE_OBJ(StateComponent, void(0));

	TickBuckets[NextBucketToAssign].Components.Add(StateComponent);
	ComponentToBucketMap.Add({StateComponent, NextBucketToAssign});

	IncrementAssignmentCounter();
}

void UAdaGameplayStateManager::UnregisterStateComponent(UAdaGameplayStateComponent* StateComponent)
{
	A_VALIDATE_OBJ(StateComponent, void(0));

	uint8* FoundBucket = ComponentToBucketMap.Find(StateComponent);
	if (!FoundBucket)
	{
		return;
	}

	TickBuckets[*FoundBucket].Components.Remove(StateComponent);
}

const UCurveFloat* UAdaGameplayStateManager::GetCurveForModifier(const FGameplayTag CurveTag)
{
	const UDataRegistrySubsystem* const DataRegistrySubsystem = UDataRegistrySubsystem::Get();
	A_ENSURE_RET(IsValid(DataRegistrySubsystem), nullptr);
	
	const FAdaAttributeCurveModifierRow* const CurveModifierRow = DataRegistrySubsystem->GetCachedItem<FAdaAttributeCurveModifierRow>({"CurveModifiers", CurveTag.GetTagName()});
	A_ENSURE_RET(CurveModifierRow, nullptr);

	return CurveModifierRow->Curve;
}

void UAdaGameplayStateManager::FixedTick(const uint64& CurrentFrame)
{
	FTickBucket& BucketToTick = TickBuckets[NextBucketToTick];
	for (TWeakObjectPtr<UAdaGameplayStateComponent> ComponentWeak : BucketToTick.Components)
	{
		UAdaGameplayStateComponent* ComponentToTick = ComponentWeak.Get();
		if (!IsValid(ComponentToTick))
		{
			continue;
		}
		
		ComponentToTick->FixedTick(BucketToTick.CurrentFrame);
	}

	BucketToTick.CurrentFrame++;

	IncrementTickCounter();
}

void UAdaGameplayStateManager::IncrementAssignmentCounter()
{
	NextBucketToAssign = (NextBucketToAssign < ADA_TICK_BUCKET_COUNT - 1) ? NextBucketToAssign + 1 : 0;
}

void UAdaGameplayStateManager::IncrementTickCounter()
{
	NextBucketToTick = (NextBucketToTick < ADA_TICK_BUCKET_COUNT - 1) ? NextBucketToTick + 1 : 0;
}