// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "AdaGameplayEditor.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"

#include "AssetTypeActions_StatusEffect.h"

#define LOCTEXT_NAMESPACE "FAdaGameplayModule"

void FAdaGameplayEditorModule::StartupModule()
{
	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	TSharedRef<IAssetTypeActions> GABAction = MakeShareable(new FAssetTypeActions_StatusEffect());
	AssetTools.RegisterAssetTypeActions(GABAction);
	CreatedAssetTypeActions.Add(GABAction);
}

void FAdaGameplayEditorModule::ShutdownModule()
{
	// Unregister asset type actions
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto& AssetTypeAction : CreatedAssetTypeActions)
		{
			if (AssetTypeAction.IsValid())
			{
				AssetToolsModule.UnregisterAssetTypeActions(AssetTypeAction.ToSharedRef());
			}
		}
	}
	
	CreatedAssetTypeActions.Empty();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAdaGameplayEditorModule, AdaGameplayEditor)