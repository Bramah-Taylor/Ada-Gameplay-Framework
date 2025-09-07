// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "AdaMessagingSubsystem.generated.h"

class UAdaMessagingSubsystem;

USTRUCT(BlueprintType)
struct FAdaMessageListenerHandle
{
	GENERATED_BODY()

public:
	FAdaMessageListenerHandle() {}

	bool IsValid() const { return ID != 0 && Channel != NAME_None; }

private:
	UPROPERTY(Transient)
	FName Channel = NAME_None;

	UPROPERTY(Transient)
	int32 ID = 0;

	FDelegateHandle StateClearedHandle;

	friend UAdaMessagingSubsystem;

	FAdaMessageListenerHandle( const FName InChannel, const int32 InID) : Channel(InChannel), ID(InID) {}
};

DECLARE_LOG_CATEGORY_EXTERN(LogAdaMessagingSubsystem, Log, All);

// Simple messaging system derived from Lyra's gameplay message router.
// Message broadcasters and listeners explicitly register to a specific message type instead of a gameplay tag channel as this gives us type safety
// and simplifies usage of the API while also making the internals of the class cleaner.
UCLASS()
class ADACORE_API UAdaMessagingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	friend struct FAdaMessageListenerHandle;
	
public:

	/// @brief	Get the current instance of the messaging subsystem.
	/// @return The message subsystem for the game instance associated with the world of the specified object.
	static UAdaMessagingSubsystem* Get(const UObject* WorldContextObject);

	// Begin UGameInstanceSubsystem overrides.
	virtual void Deinitialize() override;
	// End UGameInstanceSubsystem overrides.

	/// @brief	Broadcast a message of the specified type.
	/// @param	Message			The message to send to listeners.
	template <typename FMessageStructType>
	void BroadcastMessage(const FMessageStructType& Message)
	{
		const UScriptStruct* StructType = TBaseStructure<FMessageStructType>::Get();
		BroadcastMessageInternal(StructType, &Message);
	}

	/// @brief	Register a callback to receive specific messages
	/// @param	Callback		Function to call with the message when someone broadcasts it.
	/// @return	A handle that can be used to unregister this listener.
	template <typename FMessageStructType>
	FAdaMessageListenerHandle RegisterListener(TFunction<void(const FMessageStructType&)>&& Callback)
	{
		auto ThunkCallback = [InnerCallback = MoveTemp(Callback)](const UScriptStruct* SenderStructType, const void* SenderPayload)
		{
			InnerCallback(*static_cast<const FMessageStructType*>(SenderPayload));
		};

		const UScriptStruct* StructType = TBaseStructure<FMessageStructType>::Get();
		return RegisterListenerInternal(ThunkCallback, StructType);
	}

	/// @brief	Register to receive messages of a specified type and handle it with a specified member function.
	///			Executes a weak object validity check to ensure the object registering the function still exists before triggering the callback.
	/// @param	Object			The object instance to call the function on.
	/// @param	Function		Member function to call with the message when someone broadcasts it.
	/// @return A handle that can be used to unregister this listener.
	template <typename FMessageStructType, typename TOwner = UObject>
	FAdaMessageListenerHandle RegisterListener(TOwner* Object, void(TOwner::* Function)(const FMessageStructType&))
	{
		TWeakObjectPtr<TOwner> WeakObject(Object);
		return RegisterListener<FMessageStructType>([WeakObject, Function](const FMessageStructType& Payload)
			{
				if (TOwner* StrongObject = WeakObject.Get())
				{
					(StrongObject->*Function)(Payload);
				}
			});
	}

	/// @brief	Remove a message listener previously registered by RegisterListener.
	/// @param	Handle			The handle returned by RegisterListener.
	void UnregisterListener(FAdaMessageListenerHandle& Handle);

protected:
	/// @brief	Broadcast a message on the specified channel.
	/// @param Message			The message to send.
	UFUNCTION(BlueprintCallable, CustomThunk, Category = Messaging, Meta = (CustomStructureParam = "Message", AllowAbstract = "false", DisplayName = "Broadcast Message"))
	void K2_BroadcastMessage(const int32& Message);

	DECLARE_FUNCTION(execK2_BroadcastMessage);

private:
	// Internal helper for broadcasting a message.
	void BroadcastMessageInternal(const UScriptStruct* StructType, const void* MessageBytes);

	// Internal helper for registering a message listener.
	FAdaMessageListenerHandle RegisterListenerInternal(
		TFunction<void(const UScriptStruct*, const void*)>&& Callback,
		const UScriptStruct* StructType);

	void UnregisterListenerInternal(const FName Channel, const int32 HandleID);

private:
	struct FMessageListenerData
	{
		// Callback for when a message has been received.
		TFunction<void(const UScriptStruct*, const void*)> ReceivedCallback;

		int32 HandleID = 0;
	};

	// List of all entries for a given channel.
	struct FChannelListenerList
	{
		TArray<FMessageListenerData> Listeners;
		int32 HandleID = 0;
	};

	// We still use 'channels' internally using script struct FNames for safe struct type identification.
	TMap<FName, FChannelListenerList> ListenerMap;

	// #TODO: Add actor channels using FObjectKey
};