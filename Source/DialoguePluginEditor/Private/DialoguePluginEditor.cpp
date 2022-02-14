// Copyright Epic Games, Inc. All Rights Reserved.

#include "DialoguePluginEditor.h"

#include "Dialogue.h"
#include "DialogueExecutor.h"

#include <AssetToolsModule.h>

#include "Asset/AssetTypeActions_Dialogue.h"

#include "Editor/EditorCommands_Dialogue.h"
#include "DialogueStyle.h"

#include <EdGraphUtilities.h>
#include "DialogueEditorSettings.h"
#include "Customizations/DialogueNodeCustomization.h"
#include "Editor/EdGraphNode_DialogueNode.h"
#include "Customizations/DialogueParticipantCustomization.h"
#include "DialogueParticipantRegistry.h"
#include "Customizations/ExecutorSetupCustomization.h"




#define LOCTEXT_NAMESPACE "FDialoguePluginEditor"



#define REG_CUSTOMIZATION(PropertyModule, Target, Customization) \
PropertyModule.RegisterCustomPropertyTypeLayout(RegisteredCustomizations.Add_GetRef(Target::StaticStruct()->GetFName()), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&Customization::MakeInstance)); 

#define REG_CLASS_CUSTOMIZATION(PropertyModule, Target, Customization) \
PropertyModule.RegisterCustomClassLayout(RegisteredClassCustomizations.Add_GetRef(Target::StaticClass()->GetFName()), FOnGetDetailCustomizationInstance::CreateStatic(&Customization::MakeInstance));



class FDialoguePluginEditor : public IDialoguePluginEditor
{
	
public:
	FDialoguePluginEditor()
		: AssetCategory(EAssetTypeCategories::Misc)
	{

	}

	virtual TArray<FAssetData> GetDialogueAssets() override
	{	
		return GetParticipantRegistry()->GetAssets();
	}

	virtual TArray<FAssetData> GetDialogueBlueprints() override
	{		
		return GetParticipantRegistry()->GetBlueprints();
	}

	virtual TArray<UClass*> GetDialogueClasses() override
	{		
		return GetParticipantRegistry()->GetClasses();
	}

	void StartupModule() override
	{
		// Register slate style overrides
		FDialogueStyle::Initialize();

		// Register commands
		FDialogueEditorCommands::Register();

		//GraphPanelNodeFactory_Dialogue = MakeShareable(new FGraphPanelNodeFactory_Dialogue());
		//FEdGraphUtilities::RegisterVisualNodeFactory(GraphPanelNodeFactory_Dialogue);

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();


		if (!UDialogueEditorSettings::Get()->AssetCategory.IsEmpty())
		{
			AssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("DialogueCategory")), UDialogueEditorSettings::Get()->AssetCategory);
		}		

		AssetTools.RegisterAssetTypeActions(RegisteredAssetActions.Add_GetRef(MakeShareable(new FAssetTypeActions_Dialogue(AssetCategory))));

		// Register customizations
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >("PropertyEditor");
			REG_CUSTOMIZATION(PropertyModule, FDialogueParticipant, FDialogueParticipantCustomization);
			REG_CUSTOMIZATION(PropertyModule, FExecutorSetup, FExecutorSetupCustomization);
			REG_CLASS_CUSTOMIZATION(PropertyModule, UEdGraphNode_DialogueNode, FDialogueNodeCustomization);
		}
	}


	void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			for (const TSharedRef<IAssetTypeActions>& TypeAction : RegisteredAssetActions)
			{
				AssetTools.UnregisterAssetTypeActions(TypeAction);
			}
		}

		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

			for (FName Name : RegisteredCustomizations)
			{
				PropertyModule.UnregisterCustomPropertyTypeLayout(Name);
			}
			RegisteredCustomizations.Empty();

			for (FName Name : RegisteredClassCustomizations)
			{
				PropertyModule.UnregisterCustomClassLayout(Name);
			}
			RegisteredClassCustomizations.Empty();
		}

		RegisteredAssetActions.Empty();

		// Unregister commands
		FDialogueEditorCommands::Unregister();

		// Unregister slate style overrides
		FDialogueStyle::Shutdown();
	}

private:

	TSharedPtr<FDialogueParticipantRegistry> GetParticipantRegistry()
	{
		if (!ParticipantRegistry.IsValid())
		{
			ParticipantRegistry = MakeShared<FDialogueParticipantRegistry>();
			ParticipantRegistry->Initialize();
		}
		return ParticipantRegistry;
	}	

private:
	EAssetTypeCategories::Type AssetCategory;

	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetActions;

	TArray<FName> RegisteredCustomizations;
	TArray<FName> RegisteredClassCustomizations;


	TSharedPtr<FDialogueParticipantRegistry> ParticipantRegistry;
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDialoguePluginEditor, DialoguePluginEditor)