#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph_Dialogue.generated.h"

class UDialogue;
class UEdGraphNode_DialogueBase;
class UEdGraphNode_DialogueEntry;

UCLASS()
class DIALOGUEPLUGINEDITOR_API UEdGraph_Dialogue : public UEdGraph
{
	GENERATED_BODY()


public:
	UEdGraph_Dialogue();
	virtual ~UEdGraph_Dialogue();

	virtual bool Modify(bool bAlwaysMarkDirty = true) override;
	virtual void PostEditUndo() override;

	UDialogue* GetDialogue() const;

	void OnCreated();
	void OnLoaded();
	void Initialize();

	void OnAssetChanged();

	void RebuildDialogueGraph();
	void RebuildGraphAroundNode(UEdGraphNode_DialogueBase* Node);
	void SpawnMissingNodes();

	void FixEntryName(UEdGraphNode_DialogueEntry* EntryNode);
	void FixEntryName(UEdGraphNode_DialogueEntry* EntryNode, TMap<FName, UEdGraphNode_DialogueEntry*>& UsedNames);
	
};
