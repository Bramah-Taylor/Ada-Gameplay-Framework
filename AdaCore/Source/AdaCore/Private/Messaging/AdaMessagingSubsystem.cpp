// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "Messaging/AdaMessagingSubsystem.h"

#include "Debug/AdaAssertionMacros.h"

DEFINE_LOG_CATEGORY(LogAdaMessagingSubsystem);

UAdaMessagingSubsystem* UAdaMessagingSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	A_ENSURE_RET(IsValid(World), nullptr);
	
	return UGameInstance::GetSubsystem<UAdaMessagingSubsystem>(World->GetGameInstance());
}

void UAdaMessagingSubsystem::Deinitialize()
{
	ListenerMap.Reset();

	Super::Deinitialize();
}

void UAdaMessagingSubsystem::BroadcastMessageInternal(const UScriptStruct* StructType, const void* MessageBytes, const AActor* const Actor)
{
	const FName Channel = FName(StructType->GetStructCPPName());
	const FObjectKey ActorKey = FObjectKey(Actor);
	TListenerMap& RelevantListenerMap = (IsValid(Actor)) ? ActorChannels.FindOrAdd(ActorKey) : ListenerMap;
	
	// Broadcast the message
	if (const FChannelListenerList* List = RelevantListenerMap.Find(Channel))
	{
		// Copy in case there are removals while handling callbacks
		TArray<FMessageListenerData> ListenerArray(List->Listeners);

		for (const FMessageListenerData& Listener : ListenerArray)
		{
			Listener.ReceivedCallback(StructType, MessageBytes);
		}
	}
}

void UAdaMessagingSubsystem::K2_BroadcastMessage(const int32& Message, const AActor* const Actor)
{
	// This will never be called, the exec version below will be hit instead
	checkNoEntry();
}

DEFINE_FUNCTION(UAdaMessagingSubsystem::execK2_BroadcastMessage)
{
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const void* const MessagePtr = Stack.MostRecentPropertyAddress;
	const FStructProperty* const StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);

	P_GET_OBJECT(AActor, Actor);

	P_FINISH;

	if (ensure((StructProp != nullptr) && (StructProp->Struct != nullptr) && (MessagePtr != nullptr)))
	{
		P_THIS->BroadcastMessageInternal(StructProp->Struct, MessagePtr, Actor);
	}
}

FAdaMessageListenerHandle UAdaMessagingSubsystem::RegisterListenerInternal(TFunction<void(const UScriptStruct*, const void*)>&& Callback, const UScriptStruct* StructType, const AActor* const Actor)
{
	const FName Channel = FName(StructType->GetStructCPPName());

	const FObjectKey ActorKey = FObjectKey(Actor);
	TListenerMap& RelevantListenerMap = (IsValid(Actor)) ? ActorChannels.FindOrAdd(ActorKey) : ListenerMap;
	
	FChannelListenerList& List = RelevantListenerMap.FindOrAdd(Channel);

	FMessageListenerData& Entry = List.Listeners.AddDefaulted_GetRef();
	Entry.ReceivedCallback = MoveTemp(Callback);
	Entry.HandleID = ++List.HandleID;

	return FAdaMessageListenerHandle(Channel, Entry.HandleID);
}

void UAdaMessagingSubsystem::UnregisterListener(FAdaMessageListenerHandle& Handle)
{
	if (Handle.IsValid())
	{
		Handle.ID = 0;
		Handle.Channel = NAME_None;
		
		UnregisterListenerInternal(Handle.Channel, Handle.ID);
	}
	else
	{
		UE_LOG(LogAdaMessagingSubsystem, Warning, TEXT("Trying to unregister an invalid handle."));
	}
}

void UAdaMessagingSubsystem::UnregisterListenerInternal(const FName Channel, const int32 HandleID)
{
	if (FChannelListenerList* ChannelListeners = ListenerMap.Find(Channel))
	{
		int32 MatchIndex = ChannelListeners->Listeners.IndexOfByPredicate([ID = HandleID](const FMessageListenerData& Other) { return Other.HandleID == ID; });
		if (MatchIndex != INDEX_NONE)
		{
			ChannelListeners->Listeners.RemoveAtSwap(MatchIndex);
		}

		if (ChannelListeners->Listeners.Num() == 0)
		{
			ListenerMap.Remove(Channel);
		}
	}
}