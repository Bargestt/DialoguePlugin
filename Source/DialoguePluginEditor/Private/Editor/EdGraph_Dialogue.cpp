
#include "EdGraph_Dialogue.h"
#include "Dialogue.h"
#include "EdGraphNode_DialogueBase.h"
#include "EdGraphNode_DialogueEntry.h"
#include "EdGraphNode_DialogueNode.h"
#include <EdGraph/EdGraphPin.h>



struct FDialoguePinLess
{
	UEdGraphNode* ParentNode;

	FDialoguePinLess(UEdGraphNode* InParentNode)
		: ParentNode(InParentNode)
	{ }

	FORCEINLINE bool operator()(const UEdGraphPin& PinA, const UEdGraphPin& PinB) const
	{
		const UEdGraphNode* A = PinA.GetOwningNode();
		const UEdGraphNode* B = PinB.GetOwningNode();

		bool bIsChild_A = A->NodePosY < ParentNode->NodePosY;
		bool bIsChild_B = B->NodePosY < ParentNode->NodePosY;

		if (bIsChild_A == bIsChild_B)
		{
			if (A->NodePosX == B->NodePosX)
			{
				return A->NodePosY < B->NodePosY;
			}
			return A->NodePosX < B->NodePosX;
		}

		return bIsChild_B;
	}
};


UEdGraph_Dialogue::UEdGraph_Dialogue()
{

}

UEdGraph_Dialogue::~UEdGraph_Dialogue()
{

}


bool UEdGraph_Dialogue::Modify(bool bAlwaysMarkDirty /*= true*/)
{
	bool Rtn = Super::Modify(bAlwaysMarkDirty);

	GetDialogue()->Modify();

	for (int32 Index = 0; Index < Nodes.Num(); Index++)
	{
		Nodes[Index]->Modify();
	}

	return Rtn;
}

void UEdGraph_Dialogue::PostEditUndo()
{
	Super::PostEditUndo();

	NotifyGraphChanged();
}


UDialogue* UEdGraph_Dialogue::GetDialogue() const
{
	return CastChecked<UDialogue>(GetOuter());
}


void UEdGraph_Dialogue::OnCreated()
{
	SpawnMissingNodes();
}

void UEdGraph_Dialogue::OnLoaded()
{
	
}

void UEdGraph_Dialogue::Initialize()
{
	if (const UEdGraphSchema* GraphSchema = GetSchema())
	{
		GraphSchema->ForceVisualizationCacheClear();
	}
}

void UEdGraph_Dialogue::OnAssetChanged()
{
	UDialogue* Dialogue = GetDialogue();
	for (int32 Index = 0; Index < Nodes.Num(); Index++)
	{
		if (UEdGraphNode_DialogueNode* DialogueNode = Cast<UEdGraphNode_DialogueNode>(Nodes[Index]))
		{
			Dialogue->FixNode(DialogueNode->Node);
		}
	}
}

void UEdGraph_Dialogue::ScheduleRebuild()
{
	if (GEditor && !bRebuildScheduled)
	{
		GEditor->GetTimerManager()->SetTimerForNextTick(this, &UEdGraph_Dialogue::RebuildDialogueGraph);
		bRebuildScheduled = true;
	}	
}

void UEdGraph_Dialogue::RebuildDialogueGraph()
{
	bRebuildScheduled = false;
	UDialogue* Dialogue = GetDialogue();

	TArray<UEdGraphNode_DialogueNode*> DialogueNodes;
	TArray<UEdGraphNode_DialogueEntry*> EntryNodes;

	// Collect nodes
	for (int32 Index = 0; Index < Nodes.Num(); Index++)
	{
		if (UEdGraphNode_DialogueNode* DialogueNode = Cast<UEdGraphNode_DialogueNode>(Nodes[Index]))
		{			
			DialogueNodes.Add(DialogueNode);
		}
		else if (UEdGraphNode_DialogueEntry* EntryNode = Cast<UEdGraphNode_DialogueEntry>(Nodes[Index]))
		{
			EntryNodes.Add(EntryNode);
		}		
	}

	// Build dialogue maps

	TMap<int32, FDialogueNode> NodeMap;
	TMap<FName, int32> EntryMap;

	TSet<FDialogueParticipant> Participants;

	// Dialogue nodes first pass: Generate Ids and fill map
	for (int32 Index = 0; Index < DialogueNodes.Num(); Index++)
	{
		UEdGraphNode_DialogueNode* DialogueNode = DialogueNodes[Index];

		DialogueNode->Node.NodeID = Index;
		DialogueNode->Node.Children.Empty();			
		Dialogue->FixNode(DialogueNode->Node);

		NodeMap.Add(Index, DialogueNode->Node);
	}

	// Dialogue node second pass: Collect children Ids
	for (int32 Index = 0; Index < DialogueNodes.Num(); Index++)
	{
		UEdGraphNode_DialogueNode* DialogueNode = DialogueNodes[Index];

		// Invalid links are not allowed
		ensure(DialogueNode->GetOutputPin()->LinkedTo.Remove(nullptr) == 0);

		// Sort execution order
		DialogueNode->GetOutputPin()->LinkedTo.Sort(FDialoguePinLess(DialogueNode));

		//Update node children
		for (UEdGraphPin* ChildPin : DialogueNode->GetOutputPin()->LinkedTo)
		{
			UEdGraphNode_DialogueNode* ChildDialogueNode = Cast<UEdGraphNode_DialogueNode>(ChildPin->GetOwningNode());
			if (ChildDialogueNode)
			{
				DialogueNode->Node.Children.Add(ChildDialogueNode->Node.NodeID);
			}
		}
		NodeMap[Index].Children = DialogueNode->Node.Children;
	}


	// Entries

	
	bool bMustAddEntry = DialogueNodes.Num() > 0;
// 	bool bMustAddEntry = DialogueNodes.Num() > 0 && !EntryNodes.ContainsByPredicate(
// 		[](const UEdGraphNode_DialogueEntry* Entry) { return Entry->GetOutputPin()->LinkedTo.Num() > 0; }
// 	);

	TMap<FName, UEdGraphNode_DialogueEntry*> UsedNames;
	for (int32 Index = 0; Index < EntryNodes.Num(); Index++)
	{
		UEdGraphNode_DialogueEntry* EntryNode = EntryNodes[Index];

		if (!UsedNames.Contains(EntryNode->Name)) // Do not override first users
		{
			UsedNames.Add(EntryNode->Name, EntryNode);
		}		

		// Any dialogue must have at least one entry
		if (EntryNode->GetOutputPin()->LinkedTo.Num() > 0)
		{
			bMustAddEntry = false;
		}
	}

	for (int32 Index = 0; Index < EntryNodes.Num(); Index++)
	{
		UEdGraphNode_DialogueEntry* EntryNode = EntryNodes[Index];

		// Ensure one entry is always connected
		if (bMustAddEntry && EntryNode->GetOutputPin()->LinkedTo.Num() < 1)
		{
			EntryNode->GetOutputPin()->MakeLinkTo(DialogueNodes[0]->GetInputPin());
			bMustAddEntry = false;
		}

		// Ensure all names unique		
		FixEntryName(EntryNode, UsedNames);

		// Find first valid connected node
		for (UEdGraphPin* Connected : EntryNode->GetOutputPin()->LinkedTo)
		{
			UEdGraphNode_DialogueNode* DialogueNode = Cast<UEdGraphNode_DialogueNode>(Connected->GetOwningNode());
			if (DialogueNode)
			{
				EntryMap.Add(EntryNode->Name, DialogueNode->Node.NodeID);
				break;
			}
		}
	}

	FDialogueEditorStruct Editor(Dialogue, false);
	Editor.SetNodes(NodeMap, EntryMap);
	Dialogue->PostRebuild();
}


void UEdGraph_Dialogue::UpdateChildrenOrder(UEdGraphNode_DialogueBase* MovedNode)
{
	if (!MovedNode)
	{
		return;
	}

	//TODO: Ensure is never called during copying

	bool bUpdateExecutionOrder = false;
	for (UEdGraphPin* NodePin : MovedNode->Pins)
	{
		// Sort children on parents
		if (NodePin->Direction == EGPD_Input && NodePin->LinkedTo.Num() > 0)
		{
			for (UEdGraphPin* ParentPin : NodePin->LinkedTo)
			{
				if (ParentPin->Direction == EGPD_Output)
				{
					TArray<UEdGraphPin*> PrevOrder(ParentPin->LinkedTo);					
					ParentPin->LinkedTo.Sort(FDialoguePinLess(ParentPin->GetOwningNode()));

					bUpdateExecutionOrder = bUpdateExecutionOrder || (PrevOrder != ParentPin->LinkedTo);
				}
			}
		}
		// Sort children on self
		else if (NodePin->Direction == EGPD_Output)
		{
			TArray<UEdGraphPin*> PrevOrder(NodePin->LinkedTo);
			NodePin->LinkedTo.Sort(FDialoguePinLess(NodePin->GetOwningNode()));

			bUpdateExecutionOrder = bUpdateExecutionOrder || (PrevOrder != NodePin->LinkedTo);
		}
	}

	if (bUpdateExecutionOrder)
	{
		ScheduleRebuild();
		Modify();
	}
}

void UEdGraph_Dialogue::SpawnMissingNodes()
{
	for (int32 Index = 0; Index < Nodes.Num(); Index++)
	{
		UEdGraphNode_DialogueEntry* EntryNode = Cast<UEdGraphNode_DialogueEntry>(Nodes[Index]);

		if (EntryNode)
		{
			return;
		}
	}

	FGraphNodeCreator<UEdGraphNode_DialogueEntry> NodeBuilder(*this);
	NodeBuilder.CreateNode();
	NodeBuilder.Finalize();
}

void UEdGraph_Dialogue::FixEntryName(UEdGraphNode_DialogueEntry* EntryNode)
{
	TMap<FName, UEdGraphNode_DialogueEntry*> UsedNames;
	for (int32 Index = 0; Index < Nodes.Num(); Index++)
	{
		UEdGraphNode_DialogueEntry* OtherNode = Cast<UEdGraphNode_DialogueEntry>(Nodes[Index]);

		if (OtherNode && !UsedNames.Contains(OtherNode->Name)) // Do not override first users
		{
			UsedNames.Add(OtherNode->Name, OtherNode);
		}
	}

	FixEntryName(EntryNode, UsedNames);
}

void UEdGraph_Dialogue::FixEntryName(UEdGraphNode_DialogueEntry* EntryNode, TMap<FName, UEdGraphNode_DialogueEntry*>& UsedNames)
{	
	while (true)
	{
		UEdGraphNode_DialogueEntry* NameUsedBy = UsedNames.FindRef(EntryNode->Name);

		if (NameUsedBy == nullptr || NameUsedBy == EntryNode)
		{
			UsedNames.Add(EntryNode->Name, EntryNode);
			return;
		}

		FString OldName = EntryNode->Name.ToString();

		int32 Number = 0;
		FString NameBase, Suffix;
		if (OldName.Split(TEXT("_"), &NameBase, &Suffix) && Suffix.IsNumeric())
		{
			Number = FCString::Atoi(*Suffix);
		}
		else
		{
			NameBase = OldName;
		}
		Number++;
		EntryNode->Name = FName(NameBase + TEXT("_") + FString::FromInt(Number));
	}
}


