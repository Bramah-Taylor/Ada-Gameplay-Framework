// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "Simulation/AdaTickManager.h"

#include "Simulation/AdaTickManagerSettings.h"
#include "Debug/AdaAssertionMacros.h"

void UAdaTickManager::Initialize(FSubsystemCollectionBase& Collection)
{
	const UWorld* const World = GetWorld();
	A_ENSURE_RET(IsValid(World), void());

	const UAdaTickManagerSettings* const Settings = GetDefault<UAdaTickManagerSettings>();
	A_ENSURE_RET(IsValid(Settings), void());

	if (World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Game)
	{
		PreWorldActorTickHandle = FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &UAdaTickManager::OnWorldPreActorTick);
	}
	
	FixedStepMS = 1000.0f / Settings->TargetTPS;
	bUseAggregatedTicks = Settings->bUseAggregatedTicks;
}

void UAdaTickManager::Deinitialize()
{
	const UWorld* const World = GetWorld();
	A_VALIDATE_OBJ(World, void());
	
	if (PreWorldActorTickHandle.IsValid())
	{
		FWorldDelegates::OnWorldPreActorTick.Remove(PreWorldActorTickHandle);
	}
}

void UAdaTickManager::RegisterTickFunction(const UObject* const Object, const TFunction<void(const uint64&)>& TickFunction)
{
	FTickFunction* FoundTickFunction = TickFunctions.FindByPredicate([Object](const FTickFunction& TickFunction)
	{
		return Object == TickFunction.ObjectToTick;
	});

	if (FoundTickFunction)
	{
		return;
	}

	FTickFunction NewTickFunction;
	NewTickFunction.ObjectToTick = Object;
	NewTickFunction.TickFunction = TickFunction;
	TickFunctions.Add(MoveTemp(NewTickFunction));
}

void UAdaTickManager::UnregisterTickFunction(const UObject* const Object)
{
	for (int32 i = TickFunctions.Num() - 1; i >= 0; i--)
	{
		const UObject* const FoundObject = TickFunctions[i].ObjectToTick.Get();
		if (!IsValid(FoundObject))
		{
			TickFunctions.RemoveAt(i);
			continue;
		}
		else if (FoundObject == Object)
		{
			TickFunctions.RemoveAt(i);
			break;
		}
	}
}

void UAdaTickManager::OnWorldPreActorTick(UWorld* InWorld, ELevelTick InLevelTick, float InDeltaSeconds)
{
	const float EngineFrameDeltaTimeMS = InDeltaSeconds * 1000.0f;

	UnspentTimeMS += EngineFrameDeltaTimeMS;

	uint8 TickCount = 0;

	// In practice, we end up running slightly below our frame target, but this is probably fine.
	while (UnspentTimeMS + KINDA_SMALL_NUMBER >= FixedStepMS)
	{
		UnspentTimeMS -= FixedStepMS;
		
		TickCount++;
		CurrentFrame++;

		for (const FTickFunction& TickFunction : TickFunctions)
		{
			const UObject* const ObjectToTick = TickFunction.ObjectToTick.Get();
			if (!ensureMsgf(IsValid(ObjectToTick), TEXT("%hs: Invalid object!"), __FUNCTION__))
			{
				continue;
			}

			TickFunction.TickFunction(CurrentFrame);
		}

		if (!bUseAggregatedTicks)
		{
			break;
		}
	}
}