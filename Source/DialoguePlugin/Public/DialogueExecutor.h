// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DialogueExecutor.generated.h"

class UDialogue;
struct FDialogueNode;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueNodeEvent, int32, NodeId);


/**
 * Dialogue logic execution
 */
UCLASS(Abstract, Blueprintable)
class DIALOGUEPLUGIN_API UDialogueExecutorBase : public UObject
{
	GENERATED_BODY()

	/** Flag for world context and that object was created properly */
	uint8 bWasCreated : 1;

	uint8 bWasInitialized : 1;

public:
	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnNodeEnter;

	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnNodeLeave;


	UPROPERTY()
	UDialogue* Dialogue;

	UPROPERTY()
	TMap<FName, UObject*> Participants;

public:
	UDialogueExecutorBase();
	class UWorld* GetWorld() const override;

	/** Deferred init call */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	void Initialize();	

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool WasInitialized() const;

	/** Main participant getter function. returns actual participant */
	virtual UObject* GetParticipantFromNode(const FDialogueNode* Node, bool bEnsureInterface = false) const;


	UFUNCTION(BlueprintCallable, Category = Dialogue)
	UDialogue* GetDialogue() const;

	/** aka. outer */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	UObject* GetOwner() const;


	UFUNCTION(BlueprintCallable, Category = Dialogue)
	UObject* GetNodeParticipant(int32 NodeId) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	UObject* GetParticipant(FName Name) const;

	/** @param	bOverrideExisting	Replace participant even if it's set and valid in map */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	void SetParticipant(FName Name, UObject* Participant, bool bOverrideExisting = true);

	UFUNCTION(BlueprintCallable, Category = Dialogue, meta = (DeterminesOutputType = Class, DynamicOutputParam = OutParticipants))
	void GetParticipantsOfClass(TSubclassOf<UObject> Class, TArray<UObject*>& OutParticipants);



	/** Check conditions on node without entering */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool CheckNodeCondition(int32 NodeId);


	/** 
	 * Check available nodes to enter from supplied one 
	 * Evaluates conditions and context
	 * @param	bStopOnFirst	stops after finding first available node
	 */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	TArray<int32> FindAvailableNextNodes(int32 NodeId, bool bStopOnFirst);


	/** 
	 * Node transition function, handles linked all events
	 * Does not check conditions 
	 * @param	FromNodeId	Transition begin node. If valid: NodeLeave event will be called
	 * @param	ToNodeId	Transition end node. If valid: NodeEnter event will called. Dialogue must have this node
	 * @param	NodeId		Resulting node position
	 * @return	true	if NodeId is valid
	 */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool MoveToNode(int32 FromNodeId, int32 ToNodeId, int32& FinalNodeId);


	/** 
	 * Executes all events on specified node 
	 */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	void ExecuteNodeEvents(int32 NodeId);


	/** 
	 * Process supplied text and fill arguments 
	 * Automatically replaces arguments in text using data from Node: Executor, Participant, Context
	 * Automatically calls functions that return values of type Float, Int, String, Text, Name
	 * Allowed targets: Dialogue, Executor, Participant, Context, ParticipantKey
	 * ex. {Participant.GetParticipantName}, {Context.MyFunc}, {Executor.MyFunc2}
	 */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	void FormatText(FText InText, int32 NodeId, FText& OutText);


public:
	// Events

	/** Before initialization function */
	virtual void HandleCreated();

	/** Main initialization function */
	virtual void HandleInit();

	/** Main initialization function */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnInit"))
	void ReceiveOnInit();
	

	virtual void HandleNodeLeave(int32 NodeId);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnNodeLeave"))
	void ReceiveOnNodeLeave(int32 NodeId);

	virtual void HandleNodeEnter(int32 NodeId);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnNodeEnter"))
	void ReceiveOnNodeEnter(int32 NodeId);
};


/**
 * Standard dialogue execution
 * Executes nodes one by one until encounters dead end(no available children)
 * Node execution finish must be implemented in subclasses
 *
 * Lifecycle:
 * - OnCreated
 * - OnInit
 * - DialoueExecutionBegin
 *   + NodeExecutionBegin
 *   + *Wait user call FinishNodeExecution(NextNodeId)*
 *   + NodeExecutionEnd
 *   + *Move to NextNodeId*
 *     = NodeLeave(CurrentNodeId)
 *     = NodeEnter(NextNodeId)
 *   + NodeExecutionBegin
 *
 *   ...
 *
 * - DialogueExecutionEnd
 */
UCLASS(Abstract, Blueprintable)
class DIALOGUEPLUGIN_API UDialogueExecutor : public UDialogueExecutorBase
{
	GENERATED_BODY()
	
private:
	uint8 bExecutionInProgress : 1;
	uint8 bExecutionCleanupInProgress : 1;

protected:
	UPROPERTY()
	int32 CurrentNodeId;

public:
	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnNodeExecutionBegin;

	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnNodeExecutionEnd;

	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnDialogueExecutionStarted;

	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnDialogueExecutionFinished;


public:
	UDialogueExecutor();


	/** 
	 * Start execution at entry point node
	 * Will force initialization if it wasn't called
	 * @return	true if call resulted in execution start
	 * */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool BeginExecution(FName EntryPoint);

	/**
	 * Start execution at entry node
	 * Will force initialization if it wasn't called
	 * @return	true if call resulted in execution start
	 * */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool BeginExecutionAtNode(int32 NodeId);

	bool BeginExecution_Internal(int32 NodeId, FName EntryPoint);


	/** 
	 * Start node execution and fire events.
	 * Called automatically on BeginExecution or FinishExecution
	 */
	void ExecuteCurrentNode();

	/** 
	 * Called manually.
	 * Finish execution of current node and continue to the next one
	 * Using invalid NextNodeId will finish execution 
	 */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	void FinishNodeExecution(int32 NextNodeId);

	/** Is current node is set */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	bool IsExecutionInProgress() const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	int32 GetCurrentNodeId() const;



protected:

	/** 
	 * Main execution event. Override in subclasses
	 * Handles how dialogue execution proceeds(displays widgets, notifies actors etc.)
	 */
	UFUNCTION(BlueprintNativeEvent, Category = Dialogue)
	void NodeExecutionBegin();
	virtual void NodeExecutionBegin_Implementation()
	{
		
	}

	/** Execution cleanup */
	UFUNCTION(BlueprintNativeEvent, Category = Dialogue)
	void NodeExecutionEnd();
	virtual void NodeExecutionEnd_Implementation()
	{
		
	}

protected:
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "DialoueExecutionBegin"))
	void ReceiveDialoueExecutionBegin(int32 NodeId, FName Entry);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "DialoueExecutionEnd"))
	void ReceiveDialoueExecutionEnd(int32 NodeId);
};