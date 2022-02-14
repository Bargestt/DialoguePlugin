#pragma once

#include "CoreMinimal.h"
#include "EdGraphNode_DialogueBase.h"
#include "EdGraphNode_DialogueNode.generated.h"


UCLASS(MinimalAPI)
class UEdGraphNode_DialogueNode : public UEdGraphNode_DialogueBase
{
	GENERATED_BODY()

public:	
	UPROPERTY(EditAnywhere, Category = Main, meta = (ShowOnlyInnerProperties = true))
	FDialogueNode Node;

	UEdGraphNode_DialogueNode();

	virtual int32 GetNodeId() const override { return Node.NodeID; }

	virtual void PostPasteNode() override;
	virtual int32 GetExecutionIndex(UEdGraphNode_DialogueBase* Node) const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetBodyText() const;
	virtual FText GetContextText() const override;
	virtual bool GetHasContext() const override;
	virtual FLinearColor GetNodeBodyTintColor() const override;
	virtual FLinearColor GetNodeBorderTintColor() const override;
	virtual FSlateBrush GetNodeIcon() const override;


};
