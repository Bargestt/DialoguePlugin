
#include "ExecutorSetupCustomization.h"
#include "Dialogue.h"
#include "DialogueExecutor.h"

#include "PropertyEditing.h"
#include <PropertyCustomizationHelpers.h>
#include <IPropertyUtilities.h>







#define LOCTEXT_NAMESPACE "ExecutorSetupCustomization"


void FExecutorSetupCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{	
	Setup = nullptr;
	{
		// Read struct property	
		TArray<void*> RawStructData;
		PropertyHandle->AccessRawData(RawStructData);
		if (RawStructData.Num() > 0)
		{
			Setup = (FExecutorSetup*)(RawStructData[0]);
		}
		check(Setup != nullptr);
	}

	DialoguePropertyHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExecutorSetup, Dialogue));
	DialoguePropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FExecutorSetupCustomization::DialogueChanged));
	DialogueChanged();
	
	if (Setup->bExpandStruct && !CustomizationUtils.GetPropertyUtilities()->HasClassDefaultObject())
	{
		return;
	}

	FName EntryName = NAME_None;
	PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExecutorSetup, Entry))->GetValue(EntryName);

	FString TitleString = GetNameSafe(Setup->Dialogue) + TEXT(":") + EntryName.ToString();

	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(200)
	.MaxDesiredWidth(4096)
	[
		SNew(STextBlock)
		.Font(CustomizationUtils.GetRegularFont())
		.Text(FText::FromString(*TitleString))
	];
}

void FExecutorSetupCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> ExpandStructHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExecutorSetup, bExpandStruct));
	TSharedPtr<IPropertyHandle> DialogueHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExecutorSetup, Dialogue));
	TSharedPtr<IPropertyHandle> EntryHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExecutorSetup, Entry));
	TSharedPtr<IPropertyHandle> bOverrideDefaultHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExecutorSetup, bOverrideDefault));
	TSharedPtr<IPropertyHandle> ParticipantsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FExecutorSetup, Participants));

	if (ExpandStructHandle.IsValid())
	{
		ChildBuilder.AddProperty(ExpandStructHandle.ToSharedRef());
	}

	ChildBuilder.AddProperty(DialogueHandle.ToSharedRef());

	ChildBuilder.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			EntryHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				PropertyCustomizationHelpers::MakePropertyComboBox(
					EntryHandle,
					FOnGetPropertyComboBoxStrings::CreateSP(this, &FExecutorSetupCustomization::GenerateStrings))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				EntryHandle->CreateDefaultPropertyButtonWidgets()
			]
		];	

	ChildBuilder.AddProperty(bOverrideDefaultHandle.ToSharedRef());
	ChildBuilder.AddProperty(ParticipantsHandle.ToSharedRef())
		.CustomWidget(true)
		.ValueContent()
		[
			SNew(SHorizontalBox)
			.Visibility(this, &FExecutorSetupCustomization::GetParticipantsVisibility)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
				.OnClicked(this, &FExecutorSetupCustomization::OnClickRefresh)
				.ForegroundColor(FSlateColor::UseForeground())
				.IsFocusable(false)
				[
					SNew(STextBlock).Font(CustomizationUtils.GetRegularFont()).Text(LOCTEXT("Refresh", "Refresh"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
				.OnClicked(this, &FExecutorSetupCustomization::OnClickReset)
				.ForegroundColor(FSlateColor::UseForeground())
				.IsFocusable(false)
				[
					SNew(STextBlock).Font(CustomizationUtils.GetRegularFont()).Text(LOCTEXT("Reset", "Reset"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(3.0f, 0.0f, 0.0f, 0.0f))
			[
				SNew(STextBlock)
				.Font(CustomizationUtils.GetRegularFont())
				.Text(this, &FExecutorSetupCustomization::GetMessage)
				.ToolTipText(this, &FExecutorSetupCustomization::GetTooltip)
			]
		];
}

void FExecutorSetupCustomization::GenerateStrings(TArray<TSharedPtr<FString>>& Strings, TArray<TSharedPtr<SToolTip>>& Tooltips, TArray<bool>& Bools)
{	
	TArray<FName> Names;
	if (Setup && Setup->Dialogue)
	{
		Setup->Dialogue->GetEntryMap().GenerateKeyArray(Names);
	}
	Names.Remove(NAME_None);
	Names.Insert(NAME_None, 0);

	Strings.Empty();
	for (FName Name : Names)
	{
		Strings.Add(MakeShared<FString>(Name.ToString()));
	}
}

void FExecutorSetupCustomization::DialogueChanged()
{
	check(Setup != nullptr);

	Message = TEXT("");
	MessageTooltip = TEXT("");

	if (Setup->Dialogue)
	{
		TArray<FDialogueParticipant> DialogueParticipants = Setup->Dialogue->GetAllParticipants();
		
		TMap<FName, TArray<UObject*>> ParticipantConflicts;

		TArray<FDialogueParticipant> NewParticipants;
		for (const FDialogueParticipant& Participant : DialogueParticipants)
		{
			if (Participant.Name != NAME_None)
			{
				int32 IndexOfOld = Setup->Participants.IndexOfByPredicate([&Participant](const FDialogueParticipant& Entry) { return Entry.Name == Participant.Name; });

				if (IndexOfOld >= 0)
				{
					FDialogueParticipant OldParticipant = Setup->Participants[IndexOfOld];
					Setup->Participants.RemoveAtSwap(IndexOfOld);

					NewParticipants.Add((OldParticipant.Object) ? OldParticipant : Participant);
				}
				else
				{
					NewParticipants.Add(Participant);
				}

				if (Participant.Object)
				{
					ParticipantConflicts.FindOrAdd(Participant.Name).AddUnique(Participant.Object);
				}				
			}			
		}
		Setup->Participants = NewParticipants;


		TArray<FString> Conflicts;
		for (const auto& ConflictPair : ParticipantConflicts)
		{
			if (ConflictPair.Value.Num() > 1)
			{
				Conflicts.Add(ConflictPair.Key.ToString());
			}
		}

		if (Conflicts.Num() > 0)
		{
			Message = TEXT("Multiple participants per key");
			MessageTooltip = TEXT("These keys may cause problems: ") + FString::Join(Conflicts, TEXT(", "));
		}
	}
	else
	{
		Setup->Participants.Empty();
	}
}

FReply FExecutorSetupCustomization::OnClickRefresh()
{
	DialogueChanged();
	return FReply::Handled();
}

FReply FExecutorSetupCustomization::OnClickReset()
{
	if (Setup)
	{
		Setup->Participants.Empty();
	}
	DialogueChanged();

	return FReply::Handled();
}

EVisibility FExecutorSetupCustomization::GetParticipantsVisibility() const
{
	return (Setup && Setup->Participants.Num() > 0) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
}

FText FExecutorSetupCustomization::GetMessage() const
{
	return FText::FromString(Message);
}

FText FExecutorSetupCustomization::GetTooltip() const
{
	return FText::FromString(MessageTooltip);
}

#undef LOCTEXT_NAMESPACE


