

#include "EdGraphNode_DialogueEntry.h"
#include "DialogueEditorSettings.h"
#include "EdGraph_Dialogue.h"


#define LOCTEXT_NAMESPACE "EdNode_DialogueEntry"


UEdGraphNode_DialogueEntry::UEdGraphNode_DialogueEntry()
{
	
}

#if WITH_EDITOR
void UEdGraphNode_DialogueEntry::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UEdGraphNode_DialogueEntry, Name))
	{
		UEdGraph_Dialogue* EdGraph = GetDialogueEdGraph();
		check(EdGraph != nullptr);

		EdGraph->FixEntryName(this);
	}	
}
#endif //WITH_EDITOR

void UEdGraphNode_DialogueEntry::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	UEdGraph_Dialogue* EdGraph = GetDialogueEdGraph();
	check(EdGraph != nullptr);

	EdGraph->FixEntryName(this);
}

void UEdGraphNode_DialogueEntry::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, "SingleNode", FName(), TEXT("Out"));
}	

UEdGraphPin* UEdGraphNode_DialogueEntry::GetInputPin() const
{
	return nullptr;
}

UEdGraphPin* UEdGraphNode_DialogueEntry::GetOutputPin() const
{
	return Pins[0];
}

bool UEdGraphNode_DialogueEntry::CanUserDeleteNode() const
{
	return Name != NAME_None;
}

bool UEdGraphNode_DialogueEntry::CanDuplicateNode() const
{
	return Name != NAME_None;
}

FText UEdGraphNode_DialogueEntry::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("EntryNodeTitle", "Entry");
}

FText UEdGraphNode_DialogueEntry::GetBodyText() const
{
	return (Name != NAME_None) ? FText::FromName(Name) : FText::GetEmpty();
}

FLinearColor UEdGraphNode_DialogueEntry::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}

FLinearColor UEdGraphNode_DialogueEntry::GetNodeBodyTintColor() const
{
	return UDialogueEditorSettings::Get()->EntryColor;
}

FLinearColor UEdGraphNode_DialogueEntry::GetNodeBorderTintColor() const
{
	return FLinearColor::Transparent;
}

#undef LOCTEXT_NAMESPACE
