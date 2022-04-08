

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueParticipantInterface.h"
#include "DialogueCondition.h"
#include "Dialogue.generated.h"

class UDialogueNodeContext;
class UDialogueEvent;
class UDialogueAssetContext;



/**  */
USTRUCT(BlueprintType)
struct DIALOGUEPLUGIN_API FDialogueParticipant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "DialogueParticipantInterface", MustImplement = "DialogueParticipantInterface"))
	UObject* Object;	

	FDialogueParticipant()
		: Name(NAME_None)
		, Object(nullptr)
	{ }

	FDialogueParticipant(FName InName, UObject* InObject = nullptr)
		: Name(InName)
		, Object(InObject)
	{ }

	FORCEINLINE bool operator==(const FDialogueParticipant& Other) const
	{
		return Name == Other.Name && Object == Other.Object;
	}

	FORCEINLINE friend uint32 GetTypeHash(const FDialogueParticipant& Participant)
	{
		return ::GetTypeHash(Participant.Name);
	}
};



/**  */
USTRUCT(BlueprintType)
struct DIALOGUEPLUGIN_API FDialogueNode
{
	GENERATED_BODY()
	
	/** UniqueID assigned by owner. Negative Id is invalid */
	UPROPERTY(BlueprintReadOnly)
	int32 NodeID;

	UPROPERTY(BlueprintReadWrite);
	TArray<int32> Children;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName NodeType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FText Text;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDialogueParticipant Participant;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USoundBase* Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UDialogueWave* DialogueWave;
		
	/** Additional node info */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Instanced)
	UDialogueNodeContext* Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	UDialogueCondition* Condition;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	TArray<class UDialogueEvent*> Events;

public:

	FDialogueNode()
		: NodeID(-1)
		, Sound(nullptr)
		, DialogueWave(nullptr)
		, Context(nullptr)
		, Condition(nullptr)
	{ }

	bool HasEvents() const
	{
		for (int32 Index = 0; Index < Events.Num() ; Index++)
		{
			if (Events[Index] != nullptr)
			{
				return true;
			}
		}
		return false;
	}

	void SetContextClass(UDialogue* Outer, TSubclassOf<UDialogueNodeContext> NewClass);
	void FixContext();
	bool IsEmpty() const
	{
		return Text.IsEmpty() &&
			Participant.Name == NAME_None &&
			Participant.Object == nullptr &&
			Sound == nullptr &&
			DialogueWave == nullptr &&
			Context == nullptr;
	}

	bool HasCondition() const 
	{ 
		return Condition != nullptr;
	}

	bool CheckCondition(UObject* WorldContext) const
	{
		return Condition == nullptr || Condition->CheckCondition(WorldContext);
	}
};


/**
 * Basic dialogue data asset
 */
UCLASS(Blueprintable, BlueprintType)
class DIALOGUEPLUGIN_API UDialogue : public UPrimaryDataAsset
{
	GENERATED_BODY()

	friend FDialogueEditorStruct;
public:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UEdGraph* EdGraph;
#endif

private:
	UPROPERTY()
	TMap<FName, int32> EntryPoints;

	/** NodeId-Node map. Using array is possible, but then node removal will become extremely slow */
	UPROPERTY()
	TMap<int32, FDialogueNode> Nodes;

	/** Collected unique participants from all nodes */
	UPROPERTY(VisibleAnywhere, Category = Dialogue)
	TArray<FDialogueParticipant> Participants;

public:
	/** Ensure all nodes use specified context */
	UPROPERTY(EditAnywhere, Category = Dialogue)
	bool bUniformContext;
	
	/** Use to customize dialogue to your needs */
	UPROPERTY(EditAnywhere, Category = Dialogue)
	TSubclassOf<UDialogueNodeContext> NodeContextClass;

public:
	UDialogue();

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Ensure all node properties are correct */
	virtual void FixNode(FDialogueNode& Node);

	virtual void PostRebuild() { }
#endif //WITH_EDITOR


public:
	const TMap<FName, int32>& GetEntryMap() const { return EntryPoints; }
	const TMap<int32, FDialogueNode>& GetNodeMap() const { return Nodes; }


	// BP functions

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool HasNode(int32 NodeId) const;

	/** Returns default constructed node if not found */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	FDialogueNode GetNode(int32 NodeId) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	FText GetNodeText(int32 NodeId) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	UDialogueNodeContext* GetNodeContext(int32 NodeId) const;

	/** Empty node has no Text, Participant, Sound, Context. Invalid nodes are always empty */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool IsNodeEmpty(int32 NodeId) const;


	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool HasChildren(int32 NodeId) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	int32 GetChildrenNum(int32 NodeId) const;

	/** Returns default constructed node if not found */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	FDialogueNode GetChildNode(int32 NodeId, int32 ChildIndex) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	int32 GetChildId(int32 NodeId, int32 ChildIndex) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	TArray<FDialogueNode> GetChildrenNodes(int32 NodeId) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	TArray<int32> GetChildrenIds(int32 NodeId) const;


	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool HasEntry(FName EntryName) const;

	/** Returns -1 if not found */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	int32 GetEntryId(FName EntryName) const;

	/** Returns default constructed node if not found */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	FDialogueNode GetEntryNode(FName EntryName) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	TArray<FName> GetEntries() const;



	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool IsNodeContextUniform() const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	TSubclassOf<UDialogueNodeContext> GetDefaultNodeContextClass() const;


	UFUNCTION(BlueprintCallable, Category = Dialogue)
	TArray<FDialogueParticipant> GetAllParticipants() const;	

};



/** 
 * Convenience struct to allow dialogue editing at runtime
 * Separated from asset to remove unnecessary data
 * Keeps tight NodeId assignment
 */
USTRUCT()
struct DIALOGUEPLUGIN_API FDialogueEditorStruct
{
	GENERATED_BODY()
	
private:
	UPROPERTY()
	UDialogue* Dialogue;
		
	UPROPERTY()
	int32 LastID;

	UPROPERTY()
	TArray<int32> FreeId;	

	UPROPERTY()
	bool bIdCreationInitialized;

public:

	FDialogueEditorStruct();

	FDialogueEditorStruct(UDialogue* InDialogue, bool bInitializeIdCreation = true);


	/** Required to add new nodes. Finds maximum Id and collects free Id's to assign */
	void InitializeIdCreation();


	/** Add node to the dialogue and return assigned ID */
	int32 AddNewNode(FDialogueNode Node);

	void RemoveNode(int32 NodeID);	

	/** NodeID must exist. returns true if set successfully */
	bool SetEntry(FName Name, int32 NodeID);

	/** Set node data if it exists */
	void SetNode(int32 NodeID, FDialogueNode Node);

	bool HasNode(int32 NodeID) const;

	/** Remove all nodes from dialogue except of default ones */
	void ResetDialogue();

	/** Resets dialogue data to manually set maps. Resets Id creation */
	void SetNodes(const TMap<int32, FDialogueNode>& NewNodeMap, const TMap<FName, int32>& NewEntryMap);


	const TMap<int32, FDialogueNode>& GetNodesChecked() const
	{
		check(Dialogue != nullptr);
		return Dialogue->Nodes;
	}

	const TMap<FName, int32>& GetEntriesChecked() const
	{
		check(Dialogue != nullptr);
		return Dialogue->EntryPoints;
	}
};