// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"

#include "AdaTickManager.generated.h"

UCLASS(Config = Game)
class ADAGAMEPLAY_API UAdaTickManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterTickFunction(const UObject* const Object, const TFunction<void(const uint64&)>& TickFunction);
	void UnregisterTickFunction(const UObject* const Object);

	inline uint64 GetCurrentFrame() const { return CurrentFrame; };

	template<typename T>
	void RegisterTickFunction(T* Object, void(T::*TickFunction)(const uint64&))
	{
		static_assert(TIsDerivedFrom<T, UObject>::Value);

		TWeakObjectPtr<T> WeakObject(Object);
		TFunction<void(const uint64&)> FunctionWrapper = [WeakObject, TickFunction](const uint64& CurrentFrame)
		{
			if (T* StrongObject = WeakObject.Get())
			{
				(StrongObject->*TickFunction)(CurrentFrame);
			}
		};
		
		RegisterTickFunction(Object, MoveTemp(FunctionWrapper));
	}

protected:
	void OnWorldPreActorTick(UWorld* InWorld, ELevelTick InLevelTick, float InDeltaSeconds);

private:
	struct FTickFunction
	{
		TWeakObjectPtr<const UObject> ObjectToTick;
		TFunction<void(const uint64&)> TickFunction;
	};
	
	FDelegateHandle PreWorldActorTickHandle;

	float FixedStepMS = 1000.0f / 30.0f;
	float UnspentTimeMS = 0.0f;

	uint64 CurrentFrame = 0;

	bool bUseAggregatedTicks = false;

	TArray<FTickFunction> TickFunctions;
};