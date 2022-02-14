


#include "Dialogue.h"
#include "DialogueContext.h"

void FDialogueNode::SetContextClass(UDialogue* Outer, TSubclassOf<UDialogueNodeContext> NewClass)
{
	if (!NewClass)
	{
		Context = nullptr;
	}
	else if (Context == nullptr || Context->GetClass() != NewClass)
	{
		Context = NewObject<UDialogueNodeContext>(Outer, NewClass, NAME_None, RF_Transactional);
		FixContext();
	}	
}


void FDialogueNode::FixContext()
{
	if (Context)
	{
		Context->NodeId = NodeID;
	}
}

UDialogue::UDialogue()
{
	Nodes.Add(0, FDialogueNode());
	EntryPoints.Add(NAME_None, 0);

	bUniformContext = true;
}

#if WITH_EDITOR
void UDialogue::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UDialogue::FixNode(FDialogueNode& Node)
{
	if (bUniformContext)
	{
		Node.SetContextClass(this, NodeContextClass);
	}
}
#endif //WITH_EDITOR


bool UDialogue::HasNode(int32 NodeId) const
{
	return Nodes.Contains(NodeId);
}

FDialogueNode UDialogue::GetNode(int32 NodeId) const
{
	return Nodes.FindRef(NodeId);
}

FText UDialogue::GetNodeText(int32 NodeId) const
{
	return Nodes.FindRef(NodeId).Text;
}

UDialogueNodeContext* UDialogue::GetNodeContext(int32 NodeId) const
{
	return Nodes.FindRef(NodeId).Context;
}

bool UDialogue::IsNodeEmpty(int32 NodeId) const
{
	const FDialogueNode* NodePtr = Nodes.Find(NodeId);
	
	return (NodePtr == nullptr) || NodePtr->IsEmpty();
}

bool UDialogue::HasChildren(int32 NodeId) const
{
	const FDialogueNode* NodePtr = Nodes.Find(NodeId);
	return NodePtr && NodePtr->Children.Num() > 0;
}

int32 UDialogue::GetChildrenNum(int32 NodeId) const
{
	const FDialogueNode* NodePtr = Nodes.Find(NodeId);
	return NodePtr ? NodePtr->Children.Num() : 0;
}

FDialogueNode UDialogue::GetChildNode(int32 NodeId, int32 ChildIndex) const
{
	const FDialogueNode* NodePtr = Nodes.Find(NodeId);
	return (NodePtr && NodePtr->Children.IsValidIndex(ChildIndex)) ? GetNode(NodePtr->Children[ChildIndex]) : FDialogueNode();
}

int32 UDialogue::GetChildId(int32 NodeId, int32 ChildIndex) const
{
	const FDialogueNode* NodePtr = Nodes.Find(NodeId);
	return (NodePtr && NodePtr->Children.IsValidIndex(ChildIndex)) ? NodePtr->Children[ChildIndex] : INDEX_NONE;
}

TArray<FDialogueNode> UDialogue::GetChildrenNodes(int32 NodeId) const
{
	TArray<FDialogueNode> Arr;

	const FDialogueNode* NodePtr = Nodes.Find(NodeId);
	if (NodePtr)
	{		
		Arr.Empty(NodePtr->Children.Num());
		for (int32 ChildId : NodePtr->Children)
		{
			Arr.Add(GetNode(ChildId));
		}
	}

	return Arr;
}

TArray<int32> UDialogue::GetChildrenIds(int32 NodeId) const
{
	const FDialogueNode* NodePtr = Nodes.Find(NodeId);
	if (NodePtr)
	{
		return NodePtr->Children;
	}
	return TArray<int32>();
}

bool UDialogue::HasEntry(FName EntryName) const
{
	const int32* NodeId = EntryPoints.Find(EntryName);
	return (NodeId && *NodeId >= 0);
}

int32 UDialogue::GetEntryId(FName EntryName) const
{
	const int32* NodeId = EntryPoints.Find(EntryName);
	return NodeId ? *NodeId : INDEX_NONE;
}

FDialogueNode UDialogue::GetEntryNode(FName EntryName) const
{
	return GetNode(GetEntryId(EntryName));
}

TArray<FName> UDialogue::GetEntries() const
{
	TArray<FName> Arr;
	EntryPoints.GenerateKeyArray(Arr);
	return Arr;
}

bool UDialogue::IsNodeContextUniform() const
{
	return bUniformContext;
}

TSubclassOf<UDialogueNodeContext> UDialogue::GetDefaultNodeContextClass() const
{
	return NodeContextClass;
}

TArray<FDialogueParticipant> UDialogue::GetAllParticipants() const
{
	return Participants;
}


/*--------------------------------------------
 	Editor
 *--------------------------------------------*/

FDialogueEditorStruct::FDialogueEditorStruct()
{

}


FDialogueEditorStruct::FDialogueEditorStruct(UDialogue* InDialogue, bool bInitializeIdCreation /*= true*/)
{
	Dialogue = InDialogue;
	if (bInitializeIdCreation)
	{
		InitializeIdCreation();
	}
}

void FDialogueEditorStruct::InitializeIdCreation()
{
	LastID = 0;
	FreeId.Empty();

	if (Dialogue)
	{
		for (const auto& Pair : Dialogue->Nodes)
		{
			LastID = FMath::Max(Pair.Key, LastID);
		}

		for (int32 Index = 0; Index < LastID; Index++)
		{
			if (!Dialogue->Nodes.Contains(Index))
			{
				FreeId.Add(Index);
			}
		}
	}

	bIdCreationInitialized = true;
}


int32 FDialogueEditorStruct::AddNewNode(FDialogueNode Node)
{	
	if (!Dialogue)
	{
		return INDEX_NONE;
	}

	if (!bIdCreationInitialized)
	{
		InitializeIdCreation();
	}

	int32 NewID = 0;
	if (FreeId.Num() > 0)
	{
		NewID = FreeId.Pop();
	}
	else
	{
		NewID = LastID++;
		check(NewID < MAX_int32); 
	}
	check(!Dialogue->Nodes.Contains(NewID)); // ID must not conflict	
	
	Node.NodeID = NewID;
	Node.Children.Remove(NewID); // Ensure node doesn't refer to self
	Node.FixContext();

	Dialogue->Nodes.Add(NewID, Node);
	return NewID;
}

void FDialogueEditorStruct::RemoveNode(int32 NodeID)
{
	if (Dialogue)
	{
		int32 RemovedNum = Dialogue->Nodes.Remove(NodeID);

		check(RemovedNum <= 1);
		if (RemovedNum > 0)
		{
			FreeId.Add(NodeID);
		}
	}
}

bool FDialogueEditorStruct::SetEntry(FName Name, int32 NodeID)
{
	if (Dialogue && Dialogue->Nodes.Contains(NodeID))
	{
		Dialogue->EntryPoints.Add(Name, NodeID);
		return true;
	}
	return false;
}

void FDialogueEditorStruct::SetNode(int32 NodeID, FDialogueNode Node)
{
	if (Dialogue)
	{
		FDialogueNode* NodePtr = Dialogue->Nodes.Find(NodeID);
		if (NodePtr)
		{
			Node.FixContext();
			*NodePtr = Node;
		}
	}
}

bool FDialogueEditorStruct::HasNode(int32 NodeID) const
{
	return Dialogue ? Dialogue->Nodes.Contains(NodeID) : false;
}


void FDialogueEditorStruct::ResetDialogue()
{
	if (Dialogue)
	{
		Dialogue->Nodes.Empty();
		Dialogue->EntryPoints.Empty();

		// Default dialogue nodes must be present
		Dialogue->Nodes.Add(0, FDialogueNode());
		Dialogue->EntryPoints.Add(NAME_None, 0);
	}
}

void FDialogueEditorStruct::SetNodes(const TMap<int32, FDialogueNode>& NewNodeMap, const TMap<FName, int32>& NewEntryMap)
{
	if (Dialogue)
	{
		Dialogue->Nodes = NewNodeMap;
		Dialogue->EntryPoints = NewEntryMap;

		if (!Dialogue->Nodes.Contains(0))
		{
			Dialogue->Nodes.Add(0, FDialogueNode());
		}

		if (!Dialogue->EntryPoints.Contains(NAME_None))
		{
			Dialogue->EntryPoints.Add(NAME_None, 0);
		}
		
		TSet<FDialogueParticipant> Participants;
		for (auto& NodePair : Dialogue->Nodes)
		{
			Participants.Add(NodePair.Value.Participant);
		}
		Dialogue->Participants = Participants.Array();

		bIdCreationInitialized = false;
	}
}
