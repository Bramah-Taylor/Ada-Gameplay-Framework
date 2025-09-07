# Ada Gameplay Framework
 
Framework containing various classes and utilities which I use for the development of personal projects in UE5, currently built to support UE 5.6.

Right now, the framework contains the following:

- **AdaTickManager** - A simple manager class that runs aggregated ticks on a fixed tick.
- **AdaGameplayTagCountContainer** - A trimmed-down version of the tag count container from GAS, which tracks the count of leaf tags in the container.
- **AdaGameplayStateComponent** - A component that handles attribute and tag based state, with some conceptual similarities to GAS's AbilitySystemComponent. The main differences here are that we don't have abilities, and we expose the modifier API to be used by external systems, not just by the component's internals. They can still be handled more generically via Ada's equivalent of the GameplayEffect, **AdaStatusEffect**.
- **AdaMessagingSubsystem** - A lightweight messaging subsystem, based on Lyra's gameplay message router. The main difference with this system is that we don't use gameplay tag channels and instead bind to the message struct type directly, as this is a simpler, type-safe and generally less error-prone approach.

There's a few opportunities for optimisation to reduce the amount of time spent iterating on the GameplayStateComponent which I will hopefully address in the future when the component's been battle-tested.
