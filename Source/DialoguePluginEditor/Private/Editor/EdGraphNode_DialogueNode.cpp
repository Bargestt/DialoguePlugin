

#include "EdGraphNode_DialogueNode.h"
#include "DialogueEditorSettings.h"
#include "DialogueContext.h"
#include "DialogueParticipantInterface.h"



#define LOCTEXT_NAMESPACE "EdNode_DialogueNode"


UEdGraphNode_DialogueNode::UEdGraphNode_DialogueNode()
{

}

void UEdGraphNode_DialogueNode::PostPasteNode()
{
	Super::PostPasteNode();

	//Manually duplicate because context has different outer
	if (Node.Context)
	{
		Node.Context = DuplicateObject(Node.Context, Node.Context->GetOuter());
	}
}

int32 UEdGraphNode_DialogueNode::GetExecutionIndex(UEdGraphNode_DialogueBase* InNode) const
{
	UEdGraphNode_DialogueNode* DialogueNode = Cast<UEdGraphNode_DialogueNode>(InNode);

	if (DialogueNode && DialogueNode->Node.NodeID >= 0)
	{
		return Node.Children.IndexOfByKey(DialogueNode->Node.NodeID);
	}
	return -1;
}


FText UEdGraphNode_DialogueNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Title;
	if (Node.Context && Node.Context->GetOverrideTitle(Title))
	{
		return FText::FromString(Title);
	}

	if (Node.Participant.Object && Node.Participant.Object->Implements<UDialogueParticipantInterface>())
	{
		return IDialogueParticipantInterface::Execute_GetParticipantName(Node.Participant.Object);
	}
	else if (Node.Participant.Name != NAME_None)
	{
		Title = Node.Participant.Name.ToString();
	}

	return FText::FromString(Title);
}

FText UEdGraphNode_DialogueNode::GetTooltipText() const
{
	return Super::GetTooltipText();
}

FText UEdGraphNode_DialogueNode::GetBodyText() const
{
	FText BodyText = Node.Text;

	TArray<FString> Lines;
	BodyText.ToString().ParseIntoArray(Lines, TEXT("\n"), false);

	int32 MaxLineNum = UDialogueEditorSettings::Get()->MaxPreviewTextLines;
	if (MaxLineNum > 0 && Lines.Num() > MaxLineNum)
	{
		FString ShortenedBody;
		for (int32 Index = 0; Index < MaxLineNum; Index++)
		{
			ShortenedBody += Lines[Index] + TEXT("\n");
		}
		ShortenedBody += TEXT("...");
		BodyText = FText::FromString(ShortenedBody);
	}

	return BodyText;
}	

FText UEdGraphNode_DialogueNode::GetContextText() const
{
	return Node.Context ? FText::FromString(Node.Context->GetContextDescripton()) : FText::GetEmpty();
}

bool UEdGraphNode_DialogueNode::GetHasContext() const
{
	return Node.Context != nullptr;
}

FLinearColor UEdGraphNode_DialogueNode::GetNodeBodyTintColor() const
{
	const UDialogueEditorSettings* Settings = UDialogueEditorSettings::Get();

	EColorSource Source = Settings->NodeBodyColorSource;
	FLinearColor Color = (Node.NodeType.IsNone() || !Settings->CustomTypeNodeColor.Contains(Node.NodeType)) ? Settings->NodeColor : Settings->CustomTypeNodeColor[Node.NodeType];

	if ( (Source == EColorSource::Both || Source == EColorSource::Context)
		&& Node.Context && Node.Context->GetOverrideNodeColor(Color))
	{
		return Color;
	}

	if ((Source == EColorSource::Both || Source == EColorSource::Participant)
		&& Node.Participant.Object && Node.Participant.Object->Implements<UDialogueParticipantInterface>())	
	{
		return IDialogueParticipantInterface::Execute_GetParticipantColor(Node.Participant.Object);
	}

	return Color;
}

FLinearColor UEdGraphNode_DialogueNode::GetNodeBorderTintColor() const
{
	const UDialogueEditorSettings* Settings = UDialogueEditorSettings::Get();

	EColorSource Source = Settings->NodeBorderColorSource;
	FLinearColor Color = (Node.NodeType.IsNone() || !Settings->CustomTypeNodeBorderColor.Contains(Node.NodeType)) ? Settings->NodeColor : Settings->CustomTypeNodeBorderColor[Node.NodeType];

	if ((Source == EColorSource::Both || Source == EColorSource::Context)
		&& Node.Context && Node.Context->GetOverrideNodeColor(Color))
	{
		return Color;
	}

	if ((Source == EColorSource::Both || Source == EColorSource::Participant)
		&& Node.Participant.Object && Node.Participant.Object->Implements<UDialogueParticipantInterface>())	
	{
		return IDialogueParticipantInterface::Execute_GetParticipantColor(Node.Participant.Object);
	}

	return Color;
}

FSlateBrush UEdGraphNode_DialogueNode::GetNodeIcon() const
{
	FSlateBrush Icon = Super::GetNodeIcon();

	UObject* Participant = Node.Participant.Object;
	if (Participant && Participant->Implements<UDialogueParticipantInterface>())
	{
		Icon = IDialogueParticipantInterface::Execute_GetParticipantIcon(Participant);
	}

	FSlateBrush OverrideIcon;
	if (Node.Context && Node.Context->GetOverrideIcon(OverrideIcon))
	{
		Icon = OverrideIcon;
	}

	return Icon;
}


#undef LOCTEXT_NAMESPACE
