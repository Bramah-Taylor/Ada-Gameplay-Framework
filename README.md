# Ada Gameplay Framework
 
Framework containing various classes and utilities which I use for the development of personal projects in UE5, currently built to support UE 5.7.4

Right now, the framework contains the following:

- **AdaTickManager** - A simple manager class that runs aggregated ticks on a fixed tick.
- **AdaGameplayTagCountContainer** - A trimmed-down version of the tag count container from GAS, which tracks the count of leaf tags in the container.
- **AdaGameplayStateComponent** - A component that handles attribute and tag based state, with some conceptual similarities to GAS's AbilitySystemComponent. The main differences here are that we don't have abilities, and we expose the modifier API to be used by external systems, not just by the component's internals. They can still be handled more generically via Ada's equivalent of the GameplayEffect, **AdaStatusEffect**.
- **AdaMessagingSubsystem** - A lightweight messaging subsystem, based on Lyra's gameplay message router. The main difference with this system is that we don't use gameplay tag channels and instead bind to the message struct type directly, as this is a simpler, type-safe and generally less error-prone approach.
- **AdaAwaitSubsystem** - A system designed to simplify and decouple logic that runs pending the completion of complex conditions. Useful for situations where you have code that's dependent on multiple conditions where the order isn't guaranteed (e.g. a mix of replicated properties and server RPCs), or the code required to wait for multiple conditions to complete would be overly complex.
- **AdaLockList** - A simple wrapper for FGameplayTagContainer which acts as a lock list that gates locking based on user-specified reasons. Useful for features where they can be locked or temporarily disabled by multiple systems, such as pausing or player movement.
- **AdaDelegateStack** - A container that represents a stack of callbacks that get broadcast one at a time, LIFO, for when you don't want all the callbacks in a delegate's invocation list to get called in a single broadcast.

There's a few opportunities for optimisation to reduce the amount of time spent iterating on the GameplayStateComponent which I will hopefully address in the future when the component's been battle-tested.
