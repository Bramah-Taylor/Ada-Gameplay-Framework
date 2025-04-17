// Copyright Matt Bramah-Taylor, 2025. All Rights Reserved.

#include "AdaGameplayEditor.h"

#include "AdaAssetTypeActions_StatusEffect.h"

#define LOCTEXT_NAMESPACE "FAdaGameplayModule"

void FAdaGameplayEditorModule::StartupModule()
{
	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	StatusEffectAssetTypeAction = MakeShared<FAdaAssetTypeActions_StatusEffect>();
	AssetTools.RegisterAssetTypeActions(StatusEffectAssetTypeAction.ToSharedRef());
}

void FAdaGameplayEditorModule::ShutdownModule()
{
	// Unregister asset type actions
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetToolsModule.UnregisterAssetTypeActions(StatusEffectAssetTypeAction.ToSharedRef());
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAdaGameplayEditorModule, AdaGameplayEditor)