#pragma once

#include "CoreMinimal.h"
#include "EdGraphNode_DialogueBase.h"
#include "EdGraphNode_DialogueEntry.generated.h"


UCLASS(MinimalAPI)
class UEdGraphNode_DialogueEntry : public UEdGraphNode_DialogueBase
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, Category = Main)
	FName Name;	

public:
	UEdGraphNode_DialogueEntry();

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR

	virtual void PostPlacedNewNode() override;
	virtual void AllocateDefaultPins() override;
	virtual UEdGraphPin* GetInputPin() const override;
	virtual UEdGraphPin* GetOutputPin() const override;
	virtual bool CanUserDeleteNode() const override;
	virtual bool CanDuplicateNode() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetBodyText() const;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FLinearColor GetNodeBodyTintColor() const override;
	virtual FLinearColor GetNodeBorderTintColor() const override;
};
