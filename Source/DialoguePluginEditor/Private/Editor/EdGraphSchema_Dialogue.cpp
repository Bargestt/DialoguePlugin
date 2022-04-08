

#include "EdGraphSchema_Dialogue.h"

#include "EdGraphNode_DialogueBase.h"
#include "EdGraphNode_DialogueEntry.h"
#include "EdGraphNode_DialogueNode.h"

#include "ConnectionDrawingPolicy_Dialogue.h"

#include "Dialogue.h"
#include "DialogueParticipantInterface.h"
#include "DialogueEditorSettings.h"

#include "ToolMenus.h"
#include "GraphEditorActions.h"
#include "Framework/Commands/GenericCommands.h"

#include <ObjectEditorUtils.h>
#include "DialoguePluginEditor.h"



#define LOCTEXT_NAMESPACE "AssetSchema_Dialogue"


int32 UEdGraphSchema_Dialogue::CurrentCacheRefreshID = 0;


/*--------------------------------------------
 	FEdGraphSchemaAction_Dialogue_NewNode
 *--------------------------------------------*/


UEdGraphNode* FEdGraphSchemaAction_Dialogue_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /*= true*/)
{
	UEdGraphNode* ResultNode = nullptr;

	if (NodeTemplate != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("DialogueEditorNewNode", "Generic Graph Editor: New Node"));
		ParentGraph->Modify();
		if (FromPin != nullptr)
			FromPin->Modify();

		NodeTemplate->Rename(nullptr, ParentGraph);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		NodeTemplate->NodePosX = Location.X;
		NodeTemplate->NodePosY = Location.Y;
		
		NodeTemplate->SetFlags(RF_Transactional);

		ResultNode = NodeTemplate;
	}

	return ResultNode;
}

void FEdGraphSchemaAction_Dialogue_NewNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(NodeTemplate);
}




EGraphType UEdGraphSchema_Dialogue::GetGraphType(const UEdGraph* TestEdGraph) const
{
	return GT_StateMachine;
}

FLinearColor UEdGraphSchema_Dialogue::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FColor::White;
}

void UEdGraphSchema_Dialogue::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));
	Super::BreakNodeLinks(TargetNode);
}

void UEdGraphSchema_Dialogue::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));
	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
}

void UEdGraphSchema_Dialogue::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));
	Super::BreakSinglePinLink(SourcePin, TargetPin);
}

UEdGraphPin* UEdGraphSchema_Dialogue::DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const
{
	UEdGraphNode_DialogueBase* EdNode = Cast<UEdGraphNode_DialogueBase>(InTargetNode);
	switch (InSourcePinDirection)
	{
	case EGPD_Input:
		return EdNode->GetOutputPin();
	case EGPD_Output:
		return EdNode->GetInputPin();
	default:
		return nullptr;
	}
}

bool UEdGraphSchema_Dialogue::SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const
{
	return Cast<UEdGraphNode_DialogueBase>(InTargetNode) != nullptr;
}

bool UEdGraphSchema_Dialogue::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const
{
	return CurrentCacheRefreshID != InVisualizationCacheID;
}

int32 UEdGraphSchema_Dialogue::GetCurrentVisualizationCacheID() const
{
	return CurrentCacheRefreshID;
}

void UEdGraphSchema_Dialogue::ForceVisualizationCacheClear() const
{
	++CurrentCacheRefreshID;
}


class FConnectionDrawingPolicy* UEdGraphSchema_Dialogue::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	return new FConnectionDrawingPolicy_Dialogue(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}




void UEdGraphSchema_Dialogue::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UDialogue* Graph = CastChecked<UDialogue>(ContextMenuBuilder.CurrentGraph->GetOuter());

	FText DefaultCategory = LOCTEXT("DialogueDefaultCategory", "Default");
	FText MainCategory = LOCTEXT("DialogueMainCategory", "");

	// Default entry
	{
		TSharedPtr<FEdGraphSchemaAction_Dialogue_NewNode> Action(new FEdGraphSchemaAction_Dialogue_NewNode(MainCategory, FText::FromString(TEXT("Entry")), FText::GetEmpty(), 10));
		Action->NodeTemplate = NewObject<UEdGraphNode_DialogueEntry>(ContextMenuBuilder.OwnerOfTemporaries);

		ContextMenuBuilder.AddAction(Action);
	}

	// Default node
	{
		TSharedPtr<FEdGraphSchemaAction_Dialogue_NewNode> Action(new FEdGraphSchemaAction_Dialogue_NewNode(MainCategory, FText::FromString(TEXT("Node")), FText::GetEmpty(), 10));
		Action->NodeTemplate = NewObject<UEdGraphNode_DialogueNode>(ContextMenuBuilder.OwnerOfTemporaries);

		ContextMenuBuilder.AddAction(Action);
	}


	// Template nodes
	{
		TArray<FDialogueNodeTemplate> TemplateNodes;

		const UDialogueEditorSettings* Settings = UDialogueEditorSettings::Get();	
		for (FSoftObjectPath TablePath : Settings->NodeTemplates)
		{
			UDataTable* Table = Cast<UDataTable>(TablePath.TryLoad());

			if (Table && Table->GetRowStruct() == FDialogueNodeTemplate::StaticStruct())
			{
				TArray<FDialogueNodeTemplate*> Rows;
				Table->GetAllRows<FDialogueNodeTemplate>(TEXT("DialogueEditorSchema"), Rows);

				for (FDialogueNodeTemplate* Row : Rows)
				{
					if (Row)
					{					
						TemplateNodes.Add(*Row);
					}
				}
			}
		}

		for (const FDialogueNodeTemplate& Template : TemplateNodes)
		{
			FText Category = FText::FromString(Template.DisplayCategory);
			FText DisplayName = FText::FromString(Template.DisplayName);
			FText Tooltip = FText::FromString(Template.Tooltip);

			TSharedPtr<FEdGraphSchemaAction_Dialogue_NewNode> Action(new FEdGraphSchemaAction_Dialogue_NewNode(Category, DisplayName, Tooltip, 2));
			UEdGraphNode_DialogueNode* Node = NewObject<UEdGraphNode_DialogueNode>(ContextMenuBuilder.OwnerOfTemporaries);
			Action->NodeTemplate = Node;
			Node->Node = Template.Node;

			ContextMenuBuilder.AddAction(Action);
		}
	}
	

	// Participant asset nodes
	{
		IDialoguePluginEditor& DialogueEditor = FModuleManager::LoadModuleChecked<IDialoguePluginEditor>("DialoguePluginEditor");
		TArray<FAssetData> Assets = DialogueEditor.GetDialogueAssets();

		for (const FAssetData& Data : Assets)
		{
			UObject* Asset = Data.GetAsset();
			if (!Asset)
			{
				continue;
			}

			UClass* AssetClass = Asset->GetClass();
			if (!AssetClass)
			{
				continue;
			}


			FText Name = IDialogueParticipantInterface::Execute_GetParticipantName(Asset);
			if (Name.IsEmpty())
			{
				Name = FText::FromName(Data.AssetName);
			}		

			FText ClassCategory = FObjectEditorUtils::GetCategoryText(AssetClass);
			if (ClassCategory.IsEmpty())
			{
				ClassCategory = DefaultCategory;
			}

			FText Tooltip = AssetClass->GetToolTipText();

			TSharedPtr<FEdGraphSchemaAction_Dialogue_NewNode> Action(new FEdGraphSchemaAction_Dialogue_NewNode(ClassCategory, Name, Tooltip, 0));
			UEdGraphNode_DialogueNode* Node = NewObject<UEdGraphNode_DialogueNode>(ContextMenuBuilder.OwnerOfTemporaries);
			Action->NodeTemplate = Node;
			Node->Node.Participant.Object = Asset;

			ContextMenuBuilder.AddAction(Action);
		}
	}
}

void UEdGraphSchema_Dialogue::GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	if (Context->Node)
	{
		FToolMenuSection& Section = Menu->AddSection("DialogueAssetGraphSchemaNodeActions", LOCTEXT("ClassActionsMenuHeader", "Node Actions"));
		Section.AddMenuEntry(FGenericCommands::Get().Delete);
		Section.AddMenuEntry(FGenericCommands::Get().Cut);
		Section.AddMenuEntry(FGenericCommands::Get().Copy);
		Section.AddMenuEntry(FGenericCommands::Get().Duplicate);

		Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
	}

	Super::GetContextMenuActions(Menu, Context);
}


const FPinConnectionResponse UEdGraphSchema_Dialogue::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	// Make sure the pins are not on the same node
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode", "Can't connect node to itself"));
	}

	UEdGraphNode_DialogueBase* EdNode_A = Cast<UEdGraphNode_DialogueBase>(A->GetOwningNode());
	UEdGraphNode_DialogueBase* EdNode_B = Cast<UEdGraphNode_DialogueBase>(B->GetOwningNode());

	if (EdNode_A == nullptr || EdNode_B == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError", "Invalid node class"));
	}	

	if (A->Direction == B->Direction)
	{
		if (A->Direction == EGPD_Input)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorDirection1", "Cannot connect to Input to Input"));
		}
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorDirection2", "Cannot connect to Output to Output"));
	}

	if (A->Direction == EGPD_Output && A->LinkedTo.Num() > 0 && A->GetOwningNode()->IsA<UEdGraphNode_DialogueEntry>())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, LOCTEXT("PinConnectEntry", "Replace entry"));
	}

	if (B->Direction == EGPD_Output && B->LinkedTo.Num() > 0 && B->GetOwningNode()->IsA<UEdGraphNode_DialogueEntry>())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, LOCTEXT("PinConnectEntry", "Replace entry"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("PinConnect", "Connect nodes"));
}


#undef LOCTEXT_NAMESPACE
