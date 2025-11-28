// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "Data/AdaTaggedTableRow.h"

#include "Misc/DataValidation.h"

#include "Debug/AdaAssertionMacros.h"

void FAdaTaggedTableRow::OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
{
	Super::OnDataTableChanged(InDataTable, InRowName);

	// Fully aware how hacky this is.
	// Unfortunately, the data table API is very closed-off and inextensible.
	// However, there's too much utility to automatically renaming rows when we're using tags, so
	// it's better to have this hack in than not.

#if WITH_EDITOR
	UDataTable* MutableTable = const_cast<UDataTable*>(InDataTable);
	A_ENSURE_RET(IsValid(MutableTable), void());

	FAdaTaggedTableRow* const RowData = MutableTable->FindRow<FAdaTaggedTableRow>(InRowName, __FUNCTION__);
	A_ENSURE_RET(RowData != nullptr, void());

	uint8* GenericRowData = reinterpret_cast<uint8*>(RowData);
	A_ENSURE_RET(GenericRowData != nullptr, void());

	const FName RowTagName = RowData->GetRowTag().GetTagName();
	if ((RowTagName == NAME_None || MutableTable->GetRowMap().Find(RowTagName)))
	{
		return;
	}

	MutableTable->Modify();
	MutableTable->BeginCustomEditorTransaction(RowTagName);
	MutableTable->RemoveRow(InRowName);
	MutableTable->AddRow(RowTagName, GenericRowData, MutableTable->GetRowStruct());
	MutableTable->EndCustomEditorTransaction(RowTagName);
		
	MutableTable->HandleDataTableChanged();
#endif
}

#if WITH_EDITOR
EDataValidationResult FAdaTaggedTableRow::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (GetRowTag().IsValid() == false)
	{
		Context.AddError(INVTEXT("Invalid identifier tag used for RowName."));
		Result = EDataValidationResult::Invalid;
	}
		
	return Result;
}
#endif

FGameplayTag FAdaTaggedTableRow::GetRowTag() const
{
	checkNoEntry();
	return FGameplayTag::EmptyTag;
}
