// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DialogueExecutor.generated.h"

class UDialogue;
struct FDialogueNode;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueNodeEvent, int32, NodeId);



#if WITH_EDITOR

	#define DIALOGUE_LOG_ADD(StepStruct) StepStruct.PushToLog(ExecutionLog, OnLogChanged);
	#define DIALOGUE_LOG_CLEAR() ExecutionLog.Empty();

#else

	#define DIALOGUE_LOG_ADD(StepStruct)
	#define DIALOGUE_LOG_CLEAR()

#endif //WITH_EDITOR



/**  
 *  Container with all basic parameters to begin dialogue execution
 *  Allows entry selection from dropdown menu
 *  Exposes participant binds in dialogue
 */
USTRUCT(BlueprintType)
struct DIALOGUEPLUGIN_API FExecutorSetup
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly)
	bool bExpandStruct;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UDialogue* Dialogue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Entry;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOverrideDefault;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, meta = (LockNameSelection = true))
	TArray<FDialogueParticipant> Participants;
};



/**
 * Dialogue logic execution base class with debugger
 * Grants ability to execute nodes and traverse the dialogue
 */
UCLASS(Abstract, BlueprintType)
class DIALOGUEPLUGIN_API UDialogueExecutorBase : public UObject
{
	GENERATED_BODY()
	
	/** Flag for world context and that object was created properly */
	uint8 bWasCreated : 1;

	uint8 bWasInitialized : 1;

	uint8 bTransitionInProgress : 1;

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnExecutorCreated, const UDialogueExecutorBase&);	
	static FOnExecutorCreated OnExecutorCreated;


	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnNodeEnter;

	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnNodeLeave;



	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnNodeExecutionBegin;

	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnNodeExecutionEnd;

	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnDialogueExecutionStarted;

	UPROPERTY(BlueprintAssignable, Category = Dialogue)
	FDialogueNodeEvent OnDialogueExecutionFinished;


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
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	virtual UObject* ResolveParticipant(const FDialogueParticipant& Participant) const;


	UFUNCTION(BlueprintCallable, Category = Dialogue)
	virtual void SetDialogue(UDialogue* NewDialogue);

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

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	void SetParticipantMap(const TMap<FName, UObject*>& NewParticipants, bool bOverrideExisting = true);

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



	// Events
public:
	/** Before initialization function */
	virtual void HandleCreated();


protected:
	/** Main initialization function */
	virtual void HandleInit();

	/** Main initialization function */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnInit"))
	void ReceiveOnInit();
	

	/** Called from MoveToNode */
	virtual void HandleNodeLeave(int32 NodeId);	

	/** Called from MoveToNode */
	virtual void HandleNodeEnter(int32 NodeId);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnNodeLeave"))
	void ReceiveOnNodeLeave(int32 NodeId);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnNodeEnter"))
	void ReceiveOnNodeEnter(int32 NodeId);


	
	virtual void HandleNodeExecutionBegin(int32 NodeId);

	
	virtual void HandleNodeExecutionEnd(int32 NodeId);


	// Debugger log
public:

#if WITH_EDITOR
	struct FDialogueExecutionStep
	{
		int32 NodeId;

		enum EExecutionAction
		{
			None,
			Active,
			Finished,

			EntryUnknown,
			EntryAllowed,
			EntryDenied
		};
		EExecutionAction Action;

		FDialogueExecutionStep() : FDialogueExecutionStep(-1, EExecutionAction::None)
		{ }

		FDialogueExecutionStep(int32 NodeId, EExecutionAction Action)
			: NodeId(NodeId)
			, Action(Action)
		{ }

		FORCEINLINE bool operator==(const FDialogueExecutionStep& Other) const
		{
			return this->Action == Other.Action && this->NodeId == Other.NodeId;
		}

		FORCEINLINE bool operator!=(const FDialogueExecutionStep& Other) const
		{
			return this->Action != Other.Action || this->NodeId != Other.NodeId;
		}


		void PushToLog(TArray<FDialogueExecutionStep>& Log, FSimpleMulticastDelegate& NotifyEvent)
		{
			bool bChanged = false;
			switch (Action)
			{
			
			case Active:
			case Finished:
				if (Log.Num() == 0 || Log.Last() != *this)
				{
					Log.Add(*this);
					bChanged = true;
				}
				break;
			
			case EntryUnknown:
			case EntryAllowed:
			case EntryDenied:
			{
				bool bShouldAdd = true;
				for (int32 Index = Log.Num() - 1; Index >= 0 ; Index--)
				{
					FDialogueExecutionStep& Entry = Log[Index];
					if (Entry.Action < EExecutionAction::EntryUnknown )
					{
						break;
					}
					if (Entry.NodeId == NodeId)
					{
						Entry.Action = this->Action;
						bChanged = true;
					}
				}
				if (bShouldAdd)
				{
					Log.Add(*this);
					bChanged = true;
				}
				break;
			}

			case None:
			default:
				break;
			}

			if (bChanged)
			{
				NotifyEvent.Broadcast();
			}
		}
	};

	FSimpleMulticastDelegate OnLogChanged;
	TArray<FDialogueExecutionStep> ExecutionLog;

#endif
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
	uint8 bNodeExecutionInProgress : 1;
	uint8 bNodeExecutionCleanupInProgress : 1;

protected:
	UPROPERTY()
	int32 CurrentNodeId;



public:
	UDialogueExecutor();

	virtual void SetDialogue(UDialogue* NewDialogue) override;

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

	/** Finish executing current node and stop */
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	void StopExecution();


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