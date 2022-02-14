#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "Dialogue.h"
#include "EdGraphNode_DialogueBase.generated.h"

class UEdGraph_Dialogue;
class SEdNode_DialogueNode;

UCLASS(MinimalAPI)
class UEdGraphNode_DialogueBase : public UEdGraphNode
{
	GENERATED_BODY()

protected:


public:
	uint32 bDebuggerMark_DebugEnabled : 1;

	uint32 bDebuggerMark_Active : 1;
	uint32 bDebuggerMark_Finished : 1;
	uint32 bDebuggerMark_EntryDenied : 1;
	uint32 bDebuggerMark_EntryAllowed : 1;

public:
	UEdGraphNode_DialogueBase();
	virtual ~UEdGraphNode_DialogueBase();

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR


	UEdGraph_Dialogue* GetDialogueEdGraph() const;

	virtual int32 GetNodeId() const { return INDEX_NONE; }

//~ Begin UEdGraphNode Interface
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual void PrepareForCopying() override;
	virtual void PostPasteNode() override;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual void NodeConnectionListChanged() override;

	virtual bool CanAcceptConnection(UEdGraphPin* NodePin, UEdGraphPin* FromPin) const;

	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FLinearColor GetNodeBodyTintColor() const override;
//~ End UEdGraphNode Interface

	virtual FLinearColor GetNodeBorderTintColor() const;
	virtual FSlateBrush GetNodeIcon() const;
	virtual FText GetBodyText() const;
	virtual FText GetContextText() const;
	virtual bool GetHasContext() const;

	virtual void UpdateCachedValues() { }

	virtual UEdGraphPin* GetInputPin() const;
	virtual UEdGraphPin* GetOutputPin() const;
	
	virtual int32 GetExecutionIndex(UEdGraphNode_DialogueBase* Node) const;




};
