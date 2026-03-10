// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "AdaStatusEffectFactory.h"

#include "GameplayState/AdaStatusEffectDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AdaStatusEffectFactory)

UAdaStatusEffectFactory::UAdaStatusEffectFactory()
{
	SupportedClass = UAdaStatusEffectDefinition::StaticClass();
	bCreateNew = true;
}

UObject* UAdaStatusEffectFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UAdaStatusEffectDefinition>(InParent, Class, Name, Flags, Context);
}
