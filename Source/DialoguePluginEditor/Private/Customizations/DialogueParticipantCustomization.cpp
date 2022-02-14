
#include "DialogueParticipantCustomization.h"
#include "Dialogue.h"

#include "PropertyEditing.h"
#include <PropertyCustomizationHelpers.h>
#include "DialogueEditorSettings.h"
#include "DialogueParticipantInterface.h"
#include "DialoguePluginEditor.h"




#define LOCTEXT_NAMESPACE "DialogueParticipantCustomization"


void FDialogueParticipantCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{	
	TSharedPtr<IPropertyHandle> NameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDialogueParticipant, Name));
	TSharedPtr<IPropertyHandle> ObjectHandle =  PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FDialogueParticipant, Object));

	bool bLockNameSelection = PropertyHandle->GetBoolMetaData("LockNameSelection") || !PropertyHandle->IsEditable();

	TSharedRef<SWidget> ComboBox = PropertyCustomizationHelpers::MakePropertyComboBox(NameHandle,
			FOnGetPropertyComboBoxStrings::CreateSP(this, &FDialogueParticipantCustomization::GenerateStrings));	
	ComboBox->SetEnabled(!bLockNameSelection);

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(200)
	.MaxDesiredWidth(4096)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 5.0f, 0)
		[
			SNew(SBox)
			.MinDesiredWidth(100.0f)
			[				
				ComboBox
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			bLockNameSelection ? SNullWidget::NullWidget : NameHandle->CreateDefaultPropertyButtonWidgets()
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Fill)
		[
			SNew(SBox)
			.MinDesiredWidth(250.0f)
			[
				ObjectHandle->CreatePropertyValueWidget(true)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0.0f, 0.0f, 3.0f, 0.0f))
		[
			ObjectHandle->CreateDefaultPropertyButtonWidgets()
		]
	];
}

void FDialogueParticipantCustomization::GenerateStrings(TArray<TSharedPtr<FString>>& Strings, TArray<TSharedPtr<SToolTip>>& Tooltips, TArray<bool>& Bools)
{
	TSet<FName> Names;
	Names.Add(NAME_None);
	Names.Append(UDialogueEditorSettings::Get()->StaticParticipantNames);


	IDialoguePluginEditor& DialogueEditor = FModuleManager::LoadModuleChecked<IDialoguePluginEditor>("DialoguePluginEditor");
	
	TArray<UClass*> Classes = DialogueEditor.GetDialogueClasses();
	for (UClass* Class : Classes)
	{
		if (Class && !Class->HasAnyClassFlags(CLASS_Abstract))
		{		
			UObject* CDO = Class->GetDefaultObject();
			if (CDO && CDO->Implements<UDialogueParticipantInterface>())
			{
				Names.Add(IDialogueParticipantInterface::Execute_GetParticipantKey(CDO));
			}
		}
	}

	Strings.Empty();
	for (FName Name : Names)
	{
		Strings.Add(MakeShared<FString>(Name.ToString()));
	}
}

#undef LOCTEXT_NAMESPACE


