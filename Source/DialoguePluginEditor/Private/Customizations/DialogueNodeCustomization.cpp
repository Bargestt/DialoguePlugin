
#include "DialogueNodeCustomization.h"
#include "DialogueEditorSettings.h"

#include <DetailLayoutBuilder.h>
#include <DetailCategoryBuilder.h>
#include <DetailWidgetRow.h>
#include <PropertyCustomizationHelpers.h>
#include "DialogueContext.h"
#include "Editor/EdGraphNode_DialogueNode.h"


#define LOCTEXT_NAMESPACE "DialogueNodeCustomization"


void FDialogueNodeCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	{
		for (TWeakObjectPtr<UObject>& ObjPtr : Objects)
		{
			Dialogue = ObjPtr.IsValid() ? ObjPtr->GetTypedOuter<UDialogue>() : nullptr;
			if (Dialogue.IsValid())
			{
				break;
			}
		}
	}


	IDetailCategoryBuilder& MainCat = DetailBuilder.EditCategory(TEXT("Main"), FText::GetEmpty(), ECategoryPriority::Important);


	NodeTypeProperty = DetailBuilder.GetProperty("Node.NodeType");
	NodeTypeProperty->MarkHiddenByCustomization();

	FName NodeTypeValue = NAME_None;
	NodeTypeProperty->GetValue(NodeTypeValue);
	if (UDialogueEditorSettings::Get()->CustomNodeTypes.Num() > 0 || NodeTypeValue != NAME_None)
	{
		MainCat.AddCustomRow(LOCTEXT("NodeTypeRow", "Node type"))
		.NameContent()
		[
			NodeTypeProperty->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				PropertyCustomizationHelpers::MakePropertyComboBox(
					NodeTypeProperty,
					FOnGetPropertyComboBoxStrings::CreateSP(this, &FDialogueNodeCustomization::GenerateStrings))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox)
				.Visibility(this, &FDialogueNodeCustomization::GetResetTypeVisibility)
				[
					PropertyCustomizationHelpers::MakeResetButton(FSimpleDelegate::CreateSP(this, &FDialogueNodeCustomization::ResetType))
				]				
			]
		];
		MainCat.AddCustomRow(FText::GetEmpty())
		.NameContent()[ SNew(SSpacer) ]
		.ValueContent()[ SNew(SSpacer) ];
	}	

	

	MainCat.AddProperty(DetailBuilder.GetProperty("Node.Text"));
	MainCat.AddProperty(DetailBuilder.GetProperty("Node.Participant"));

	
	if (UDialogueEditorSettings::Get()->bHideDefaultSound)
	{
		DetailBuilder.GetProperty("Node.Sound")->MarkHiddenByCustomization();
	}
	else
	{
		MainCat.AddProperty(DetailBuilder.GetProperty("Node.Sound"));
	}

	if (UDialogueEditorSettings::Get()->bHideDefaultDialogueWave)
	{
		DetailBuilder.GetProperty("Node.DialogueWave")->MarkHiddenByCustomization();
	}
	else
	{
		MainCat.AddProperty(DetailBuilder.GetProperty("Node.DialogueWave"));
	}

	MainCat.AddProperty(DetailBuilder.GetProperty("Node.Condition"));
	MainCat.AddProperty(DetailBuilder.GetProperty("Node.Events"));
	MainCat.AddProperty(DetailBuilder.GetProperty("Node.Context")).Visibility(EVisibility::Collapsed);	

	if (Dialogue.IsValid() && !Dialogue->bUniformContext)
	{
		MainCat.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			SNew(STextBlock)
			.Font(DetailBuilder.GetDetailFont())
			.Text(LOCTEXT("ContextClassPickerRow", "Context Class"))
		]
		.ValueContent()
		[
			SNew(SClassPropertyEntryBox)
			.MetaClass(Dialogue->NodeContextClass ? Dialogue->NodeContextClass : UDialogueNodeContext::StaticClass())
			.AllowAbstract(false)
			.AllowNone(true)
			.ShowDisplayNames(true)
			.SelectedClass(this, &FDialogueNodeCustomization::GetSelectedClass)
			.OnSetClass(this, &FDialogueNodeCustomization::OnSetClass)
		];
	}
}

void FDialogueNodeCustomization::OnSetClass(const UClass* Class)
{
	if (!Dialogue.IsValid() || Dialogue->bUniformContext)
	{	
		return;
	}

	for (TWeakObjectPtr<UObject> ObjPtr : Objects)
	{
		UEdGraphNode_DialogueNode* Node = Cast<UEdGraphNode_DialogueNode>(ObjPtr.Get());
		if (Node)
		{
			Node->Node.SetContextClass(Dialogue.Get(), TSubclassOf<UDialogueNodeContext>(const_cast<UClass*>(Class)));
		}
	}
}


const UClass* FDialogueNodeCustomization::GetSelectedClass() const
{
	TSet<const UClass*> Classes;

	for (TWeakObjectPtr<UObject> ObjPtr : Objects)
	{
		UEdGraphNode_DialogueNode* Node = Cast<UEdGraphNode_DialogueNode>(ObjPtr.Get());
		if (Node)
		{
			Classes.Add(Node->Node.Context ? Node->Node.Context->GetClass() : nullptr);
		}
	}

	if (Classes.Num() == 1)
	{
		return Classes.Array()[0];
	}

	return nullptr;
}

void FDialogueNodeCustomization::GenerateStrings(TArray<TSharedPtr<FString>>& Strings, TArray<TSharedPtr<SToolTip>>& Tooltips, TArray<bool>& Bools)
{
	TSet<FName> Types;
	Types.Append(UDialogueEditorSettings::Get()->CustomNodeTypes);
	
	if (Types.Remove(FName(NAME_None)) > 0)
	{		
		Strings.Add(MakeShared<FString>(FName(NAME_None).ToString()));
	}

	for (FName& Type : Types)
	{
		Strings.Add(MakeShared<FString>(Type.ToString()));
	}
}

void FDialogueNodeCustomization::ResetType()
{
	NodeTypeProperty->ResetToDefault();
}

EVisibility FDialogueNodeCustomization::GetResetTypeVisibility() const
{
	FName NodeTypeValue = NAME_None;
	NodeTypeProperty->GetValue(NodeTypeValue);
	return (NodeTypeValue != NAME_None) ? EVisibility::Visible : EVisibility::Hidden;
}

#undef LOCTEXT_NAMESPACE

