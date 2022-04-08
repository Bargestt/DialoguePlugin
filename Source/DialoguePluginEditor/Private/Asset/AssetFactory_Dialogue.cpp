

#include "Asset/AssetFactory_Dialogue.h"
#include "Dialogue.h"
#include <ClassViewerFilter.h>
#include <ClassViewerModule.h>
#include <Kismet2/SClassPickerDialog.h>
#include "DialogueEditorSettings.h"


class FAssetClassParentFilter : public IClassViewerFilter
{
public:
	FAssetClassParentFilter()
		: DisallowedClassFlags(CLASS_None)
	{}

	/** All children of these classes will be included unless filtered out by another setting. */
	TSet< const UClass* > AllowedChildrenOfClasses;

	/** Disallowed class flags. */
	EClassFlags DisallowedClassFlags;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		bool bAllowed= !InClass->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
			
		return bAllowed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}
};


UAssetFactory_Dialogue::UAssetFactory_Dialogue(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UDialogue::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}


bool UAssetFactory_Dialogue::ConfigureProperties()
{
	DialogueAssetClass = UDialogueEditorSettings::Get()->DefaultAssetClass;

	if (DialogueAssetClass != nullptr)
	{
		return true;
	}

	// Load the classviewer module to display a class picker
	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
	

	// Fill in options
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;

	TSharedRef<FAssetClassParentFilter> Filter = MakeShareable(new FAssetClassParentFilter);
	Options.ClassFilter = Filter;

	Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown;
	Filter->AllowedChildrenOfClasses.Add(UDialogue::StaticClass());

	const FText TitleText = NSLOCTEXT("DialogueAssetFactory", "CreateDataAssetOptions", "Pick Data Asset Class");
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UDialogue::StaticClass());

	if (bPressedOk)
	{
		DialogueAssetClass = ChosenClass;
	}

	return bPressedOk;
}

UObject* UAssetFactory_Dialogue::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UDialogue* NewDialogue = nullptr;
	if (DialogueAssetClass != nullptr)
	{
		NewDialogue = NewObject<UDialogue>(InParent, DialogueAssetClass, Name, Flags | RF_Transactional, Context);
	}
	else
	{
		NewDialogue = NewObject<UDialogue>(InParent, Class, Name, Flags | RF_Transactional, Context);;
	}

	if (NewDialogue)
	{
		NewDialogue->bUniformContext = UDialogueEditorSettings::Get()->bUniformContext;
		NewDialogue->NodeContextClass = UDialogueEditorSettings::Get()->DefaultNodeContext;
	}

	return NewDialogue;
}
