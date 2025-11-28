// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#pragma once

#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "AdaTaggedTableRow.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct ADACORE_API FAdaTaggedTableRow : public FTableRowBase
{
	GENERATED_BODY()
	
	// Begin FTableRowBase overrides.
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override;
#if WITH_EDITOR

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif // WITH_EDITOR
	// End FTableRowBase overrides.

	virtual FGameplayTag GetRowTag() const;
};