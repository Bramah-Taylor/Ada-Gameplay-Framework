// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "GameplayState/AdaGameplayStateManager.h"

#include "DataRegistrySubsystem.h"
#include "Engine/AssetManager.h"

#include "Simulation/AdaTickManager.h"
#include "GameplayState/AdaGameplayStateComponent.h"
#include "Debug/AdaAssertionMacros.h"

DEFINE_LOG_CATEGORY(LogAdaGameplayStateManager);

UAdaGameplayStateManager::UAdaGameplayStateManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UAdaGameplayStateManager::InitializeComponent()
{
	Super::InitializeComponent();
	
	TickBuckets.AddDefaulted(ADA_TICK_BUCKET_COUNT);

	// Ideally, this should probably go into a world subsystem, but I'd prefer to encapsulate all global gameplay state
	// functionality on this component. If this proves to be a problem, it can be moved later.
	UAssetManager& AssetManager = UAssetManager::Get();
	FStreamableDelegate StreamableDelegate = FStreamableDelegate::CreateUObject(this, &UAdaGameplayStateManager::OnStatusEffectDefsLoaded);
	AssetManager.LoadPrimaryAssetsWithType(UAdaStatusEffectDefinition::PrimaryAssetType, TArray<FName>(), StreamableDelegate, FStreamableManager::AsyncLoadHighPriority);
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

	LoadedStatusEffectDefinitions.Empty();
	
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

const UAdaStatusEffectDefinition* UAdaGameplayStateManager::GetStatusEffectDefinition(const FGameplayTag EffectTag) const
{
	const TObjectPtr<const UAdaStatusEffectDefinition>* const FoundDefPtr = LoadedStatusEffectDefinitions.Find(EffectTag);
	A_ENSURE_MSG_RET(FoundDefPtr, nullptr, TEXT("%hs: Could not find status effect definition for tag %s."), __FUNCTION__, *EffectTag.ToString());

	return *FoundDefPtr;
}

void UAdaGameplayStateManager::GetAllStatusEffectTags(TArray<FGameplayTag>& OutTags) const
{
	return LoadedStatusEffectDefinitions.GenerateKeyArray(OutTags);
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

void UAdaGameplayStateManager::OnStatusEffectDefsLoaded()
{
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<UObject*> LoadedObjects;
	AssetManager.GetPrimaryAssetObjectList(UAdaStatusEffectDefinition::PrimaryAssetType, LoadedObjects);
	for (const UObject* const LoadedObject : LoadedObjects)
	{
		if (const UAdaStatusEffectDefinition* const LoadedStatusEffect = Cast<UAdaStatusEffectDefinition>(LoadedObject))
		{
			LoadedStatusEffectDefinitions.Add(LoadedStatusEffect->EffectTag, LoadedStatusEffect);
		}
	}

	UE_LOG(LogAdaGameplayStateManager, Display, TEXT("%hs: Loaded %i status effect definitions."), __FUNCTION__, LoadedStatusEffectDefinitions.Num());
}
