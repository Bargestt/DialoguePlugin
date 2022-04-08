// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



#include "EdGraphNode_DialogueBase.h"
#include "EdGraph_Dialogue.h"

// 
// #include "Kismet2/Kismet2NameValidators.h"
// #include "Kismet2/BlueprintEditorUtils.h"

#include "Dialogue.h"
#include "SGraphNode_Dialogue.h"


#define LOCTEXT_NAMESPACE "EdNode_DialogueBase"

UEdGraphNode_DialogueBase::UEdGraphNode_DialogueBase()
{
	bCanRenameNode = false;
}

UEdGraphNode_DialogueBase::~UEdGraphNode_DialogueBase()
{

}


#if WITH_EDITOR
void UEdGraphNode_DialogueBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateCachedValues();

	UEdGraph_Dialogue* DialogueEdGraph = GetDialogueEdGraph();
	check(DialogueEdGraph != nullptr);
	DialogueEdGraph->ScheduleRebuild();
}
#endif // WITH_EDITOR




UEdGraph_Dialogue* UEdGraphNode_DialogueBase::GetDialogueEdGraph() const
{
	return Cast<UEdGraph_Dialogue>(GetGraph());
}


TSharedPtr<SGraphNode> UEdGraphNode_DialogueBase::CreateVisualWidget()
{
	return SNew(SGraphNode_Dialogue, this);
}

void UEdGraphNode_DialogueBase::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "MultipleNodes", FName(), TEXT("In"));
	CreatePin(EGPD_Output, "MultipleNodes", FName(), TEXT("Out"));
}



FText UEdGraphNode_DialogueBase::GetTooltipText() const
{
	return Super::GetTooltipText();
}

void UEdGraphNode_DialogueBase::PrepareForCopying()
{
	Super::PrepareForCopying();
}


void UEdGraphNode_DialogueBase::PostPasteNode()
{
	Super::PostPasteNode();	
}

void UEdGraphNode_DialogueBase::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr)
	{		
		UEdGraphPin* TargetPin = nullptr;
		if (FromPin->Direction == EGPD_Input)
		{
			TargetPin = GetOutputPin();
		}
		else if (FromPin->Direction == EGPD_Output)
		{
			TargetPin = GetInputPin();			
		}

		if (TargetPin && GetSchema()->TryCreateConnection(FromPin, TargetPin))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}


void UEdGraphNode_DialogueBase::NodeConnectionListChanged()
{
	UEdGraph_Dialogue* EdGraph = GetDialogueEdGraph();
	check(EdGraph != nullptr);

	EdGraph->RebuildDialogueGraph();
}

bool UEdGraphNode_DialogueBase::CanAcceptConnection(UEdGraphPin* NodePin, UEdGraphPin* FromPin) const
{
	return true;
}

FLinearColor UEdGraphNode_DialogueBase::GetNodeTitleColor() const
{	
	return Super::GetNodeTitleColor();
}

FLinearColor UEdGraphNode_DialogueBase::GetNodeBodyTintColor() const
{
	return Super::GetNodeBodyTintColor();
}

FLinearColor UEdGraphNode_DialogueBase::GetNodeBorderTintColor() const
{
	return FLinearColor::White;
}

FSlateBrush UEdGraphNode_DialogueBase::GetNodeIcon() const
{
	return FSlateNoResource();
}

FText UEdGraphNode_DialogueBase::GetBodyText() const
{
	return FText::GetEmpty();
}

FText UEdGraphNode_DialogueBase::GetContextText() const
{
	return FText::GetEmpty();
}

bool UEdGraphNode_DialogueBase::GetHasContext() const
{
	return false;
}

UEdGraphPin* UEdGraphNode_DialogueBase::GetInputPin() const
{
	return Pins[0];
}

UEdGraphPin* UEdGraphNode_DialogueBase::GetOutputPin() const
{
	return Pins[1];
}


int32 UEdGraphNode_DialogueBase::GetExecutionIndex(UEdGraphNode_DialogueBase* InNode) const
{	
	return 0;
}




#undef LOCTEXT_NAMESPACE
