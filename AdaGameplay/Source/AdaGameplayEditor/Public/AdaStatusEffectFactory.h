// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"

#include "AdaStatusEffectFactory.generated.h"

class UAdaStatusEffectDefinition;

UCLASS()
class ADAGAMEPLAYEDITOR_API UAdaStatusEffectFactory : public UFactory
{
	GENERATED_BODY()
	
public:
	UAdaStatusEffectFactory();
	
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};