// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueExecutor.h"
#include "DialoguePlugin.h"
#include "DialogueCondition.h"
#include "Dialogue.h"
#include "DialogueContext.h"
#include "DialogueParticipantInterface.h"
#include "DialogueEvent.h"


UDialogueExecutorBase::UDialogueExecutorBase()
{
	Dialogue = nullptr;
}

class UWorld* UDialogueExecutorBase::GetWorld() const
{
	return (bWasCreated && GetOuter()) ? GetOuter()->GetWorld() : nullptr;
}


void UDialogueExecutorBase::Initialize()
{
	if (bWasInitialized)
	{
		return;
	}
	bWasInitialized = true;

	HandleInit();
}


bool UDialogueExecutorBase::WasInitialized() const
{	
	return bWasInitialized;
}

UObject* UDialogueExecutorBase::GetParticipantFromNode(const FDialogueNode* Node, bool bEnsureInterface /*= false*/) const
{
	UObject* Participant = nullptr;

	if (Node)
	{
		Participant = Node->Participant.Object;
		if (Participant == nullptr && Node->Participant.Name != NAME_None)
		{
			Participant = Participants.FindRef(Node->Participant.Name);
		}

		if (bEnsureInterface && Participant && !Participant->Implements<UDialogueParticipantInterface>())
		{
			Participant = nullptr;
		}
	}

	return Participant;
}

UDialogue* UDialogueExecutorBase::GetDialogue() const
{
	return Dialogue;
}

UObject* UDialogueExecutorBase::GetOwner() const
{
	return GetOuter();
}

UObject* UDialogueExecutorBase::GetNodeParticipant(int32 NodeId) const
{
	if (Dialogue)
	{
		return GetParticipantFromNode(Dialogue->GetNodeMap().Find(NodeId));
	}
	return nullptr;
}

UObject* UDialogueExecutorBase::GetParticipant(FName Name) const
{
	return Participants.FindRef(Name);
}

void UDialogueExecutorBase::SetParticipant(FName Name, UObject* InParticipant, bool bOverrideExisting /*= true*/)
{
	UObject*& Participant = Participants.FindOrAdd(Name, nullptr);
	if (Participant == nullptr || bOverrideExisting)
	{
		Participant = InParticipant;
	}
}

void UDialogueExecutorBase::GetParticipantsOfClass(TSubclassOf<UObject> Class, TArray<UObject*>& OutParticipants)
{
	for (const auto& Pair : Participants)
	{
		if (Pair.Value && (!Class || Pair.Value->GetClass()->IsChildOf(Class)))
		{
			OutParticipants.Add(Pair.Value);
		}
	}
}

bool UDialogueExecutorBase::CheckNodeCondition(int32 NodeId)
{
	if (Dialogue)
	{
		const FDialogueNode* Node = Dialogue->GetNodeMap().Find(NodeId);
		if (Node)
		{
			return Node->Condition == nullptr || Node->Condition->CheckCondition(this);
		}
	}
	
	return false;
}

TArray<int32> UDialogueExecutorBase::FindAvailableNextNodes(int32 NodeId, bool bStopOnFirst)
{
	TArray<int32> AvailableChildren;

	if (Dialogue)
	{
		const TMap<int32, FDialogueNode>& NodeMap = Dialogue->GetNodeMap();

		TArray<int32> Children = Dialogue->GetChildrenIds(NodeId);		
		for (int32 ChildId : Children)
		{
			const FDialogueNode* Child = NodeMap.Find(ChildId);

			bool bCanEnterChild = 
				Child &&
				(Child->Context == nullptr || Child->Context->CanEnterNode(this, NodeId)) &&
				(Child->Condition == nullptr || Child->Condition->CheckCondition(this));

			if (bCanEnterChild)
			{
				AvailableChildren.Add(ChildId);

				if (bStopOnFirst)
				{
					break;;
				}
			}
		}
	}

	return AvailableChildren;
}


bool UDialogueExecutorBase::MoveToNode(int32 FromNodeId, int32 ToNodeId, int32& FinalNodeId)
{
	FinalNodeId = INDEX_NONE;
	if (Dialogue)
	{
		if (FromNodeId == ToNodeId)
		{
			FinalNodeId = ToNodeId;
		}
		else
		{
			if (FromNodeId >= 0)
			{
				HandleNodeLeave(FromNodeId);
			}

			FinalNodeId = INDEX_NONE;

			if (ToNodeId >= 0 && Dialogue->HasNode(ToNodeId))
			{
				FinalNodeId = ToNodeId;
				HandleNodeEnter(FinalNodeId);
			}
		}
	}	

	return FinalNodeId >= 0;
}


void UDialogueExecutorBase::ExecuteNodeEvents(int32 NodeId)
{
	if (Dialogue)
	{
		const FDialogueNode* Node = Dialogue->GetNodeMap().Find(NodeId);
		if (Node)
		{
			for (UDialogueEvent* Event : Node->Events)
			{
				if (Event && Event->CanExecute(this, Dialogue, NodeId))
				{
					Event->ExecuteEvent(this, Dialogue, NodeId);
				}
			}
		}
	}
}



void UDialogueExecutorBase::FormatText(FText InText, int32 NodeId, FText& OutText)
{
	FTextFormat Format(InText);

	TArray<FString> ArgumentNames;
	Format.GetFormatArgumentNames(ArgumentNames);

	FFormatNamedArguments NamedArguments;
		

	const FDialogueNode* Node = nullptr;
	UObject* Participant = nullptr;
	UDialogueNodeContext* Context = nullptr;
	if (Dialogue)
	{
		Node = Dialogue->GetNodeMap().Find(NodeId);
		Participant = GetParticipantFromNode(Node);
		Context = Node ? Node->Context : nullptr;
	}


	struct FPropertyTypeChecker
	{
		enum class EType : uint8
		{
			None,
			FString,
			FText,
			FName,
			Float,
			Integer
		};	

		EType Type = EType::None;

		void SetType(FProperty* Property)
		{
			FString CPPType = Property->GetCPPType();
			if (CPPType.Equals(TEXT("FString")))
			{
				Type = EType::FString;
			}
			else if (CPPType.Equals(TEXT("FText")))
			{
				Type = EType::FText;
			}
			else if (CPPType.Equals(TEXT("FName")))
			{
				Type = EType::FName;
			}
			else if (CPPType.Equals(TEXT("float")))
			{
				Type = EType::Float;
			}
			else if (CPPType.Equals(TEXT("int32")))
			{
				Type = EType::Integer;
			}
			else
			{
				Type = EType::None;
			}
		}
	};	
	

	for (const FString& ArgumentName : ArgumentNames)
	{
		FString TargetPart;
		FString FuncNamePart;
		if (!ArgumentName.Split(TEXT("."), &TargetPart, &FuncNamePart))
		{
			UE_LOG(LogDialogue, Error, TEXT("UDialogueExecutor::FormatText: Invalid argument %s. Argument must be in format {TargetName.FunctionName}"), *ArgumentName);
			continue;
		}
		FName TargetName = *TargetPart;

		UObject* TargetObject = nullptr;		

		if (TargetName == FName(TEXT("Executor")))
		{
			TargetObject = this;
		}
		else if (TargetName == FName(TEXT("Dialogue")))
		{
			TargetObject = Dialogue;
		}
		else if (TargetName == FName(TEXT("Participant")))
		{
			TargetObject = Participant;
		}
		else if (TargetName == FName(TEXT("Context")))
		{	
			TargetObject = Context;
		}

		//Fallback to participant in map
		if (TargetObject == nullptr)
		{
			TargetObject = Participants.FindRef(TargetName);
		}

		if (!TargetObject)
		{
			UE_LOG(LogDialogue, Error, TEXT("UDialogueExecutor::FormatText: Target not found %s"), *ArgumentName);
			continue;
		}
			
		UFunction* Func = TargetObject->FindFunction(FName(*FuncNamePart));
		if (!Func)
		{
			UE_LOG(LogDialogue, Error, TEXT("UDialogueExecutor::FormatText: Function %s not found in %s"), *FuncNamePart, *TargetObject->GetName());
			continue;
		}

		FPropertyTypeChecker ReturnType;

		bool bHasInputArguments = false;		
		int32 ReturnArguments = 0;
		for (TFieldIterator<FProperty> It(Func); It; ++It)
		{
			FProperty* Prop = *It;
			if (Prop->HasAnyPropertyFlags(CPF_Parm) && !Prop->HasAnyPropertyFlags(CPF_OutParm))
			{
				bHasInputArguments = true;
				break;
			}
			else if (Prop->HasAllPropertyFlags(CPF_Parm | CPF_OutParm))
			{
				ReturnArguments++;
				ReturnType.SetType(Prop);
			}
		}		

		if (ReturnArguments != 1)
		{
			UE_LOG(LogDialogue, Error, TEXT("UDialogueExecutor::FormatText: Function: %s: Function must have one output"), *Func->GetName());
			continue;
		}

		if (bHasInputArguments)
		{
			UE_LOG(LogDialogue, Error, TEXT("UDialogueExecutor::FormatText: Function: %s: Function must have no input"), *Func->GetName());
			continue;
		}

		if (ReturnType.Type == FPropertyTypeChecker::EType::None)
		{
			UE_LOG(LogDialogue, Error, TEXT("UDialogueExecutor::FormatText: Function: %s Unrecognized output parameter in %s"), *Func->GetName());
			continue;
		}
		
		switch (ReturnType.Type)
		{
		case FPropertyTypeChecker::EType::FString:
		{
			FString Value;
			TargetObject->ProcessEvent(Func, &Value);
			NamedArguments.Add(ArgumentName, FFormatArgumentValue(FText::FromString(Value)));
		}
			break;
		case FPropertyTypeChecker::EType::FText:
		{
			FText Value;
			TargetObject->ProcessEvent(Func, &Value);
			NamedArguments.Add(ArgumentName, FFormatArgumentValue(Value));
		}
			break;
		case FPropertyTypeChecker::EType::FName:
		{
			FName Value;
			TargetObject->ProcessEvent(Func, &Value);
			NamedArguments.Add(ArgumentName, FFormatArgumentValue(FText::FromName(Value)));
		}
			break;
		case FPropertyTypeChecker::EType::Float:
		{
			float Value;
			TargetObject->ProcessEvent(Func, &Value);
			NamedArguments.Add(ArgumentName, FFormatArgumentValue(Value));
		}
			break;
		case FPropertyTypeChecker::EType::Integer:
		{
			int32 Value;
			TargetObject->ProcessEvent(Func, &Value);
			NamedArguments.Add(ArgumentName, FFormatArgumentValue(Value));
		}
			break;
		} //Switch end
	}

	OutText = FText::Format(Format, NamedArguments);
}

void UDialogueExecutorBase::HandleCreated()
{
	bWasCreated = true;
}

void UDialogueExecutorBase::HandleInit()
{	
	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		ReceiveOnInit();
	}
}

void UDialogueExecutorBase::HandleNodeLeave(int32 NodeId)
{
	if (Dialogue)
	{
		if (const FDialogueNode* Node = Dialogue->GetNodeMap().Find(NodeId))
		{
			UObject* Participant = GetParticipantFromNode(Node, true);
			if (Participant)
			{
				IDialogueParticipantInterface::Execute_OnNodeLeft(Participant, this, Dialogue, NodeId);
			}

			if (Node->Context)
			{
				Node->Context->OnNodeLeft(this);
			}
		}
	}

	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		ReceiveOnNodeLeave(NodeId);
	}

	OnNodeLeave.Broadcast(NodeId);
}

void UDialogueExecutorBase::HandleNodeEnter(int32 NodeId)
{		
	if (Dialogue)
	{
		if (const FDialogueNode* Node = Dialogue->GetNodeMap().Find(NodeId))
		{
			UObject* Participant = GetParticipantFromNode(Node, true);
			if (Participant)
			{
				IDialogueParticipantInterface::Execute_OnNodeEntered(Participant, this, Dialogue, NodeId);
			}

			if (Node->Context)
			{
				Node->Context->OnNodeEntered(this);
			}
		}
	}

	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		ReceiveOnNodeEnter(NodeId);
	}

	OnNodeEnter.Broadcast(NodeId);
}



UDialogueExecutor::UDialogueExecutor()
{
	CurrentNodeId = -1;
}

/*--------------------------------------------
 	UDialogueExecutor
 *--------------------------------------------*/

 
bool UDialogueExecutor::BeginExecution(FName EntryPoint)
{
	return Dialogue && BeginExecution_Internal(Dialogue->GetEntryId(EntryPoint), EntryPoint);
}

bool UDialogueExecutor::BeginExecutionAtNode(int32 NodeId)
{
	return Dialogue && BeginExecution_Internal(NodeId, NAME_None);
}

bool UDialogueExecutor::BeginExecution_Internal(int32 NodeId, FName EntryPoint)
{
	if (!WasInitialized())
	{
		Initialize();
	}

	if (!Dialogue)
	{
		UE_LOG(LogDialogue, Error, TEXT("[%s] Failed to start dialogue execution: No dialogue"), *GetName());
		return false;
	}
	if (NodeId < 0)
	{
		UE_LOG(LogDialogue, Error, TEXT("[%s] Failed to start dialogue execution: Invalid node"), *GetName());
		return false;
	}
	if (bExecutionCleanupInProgress || IsExecutionInProgress())
	{
		UE_LOG(LogDialogue, Error, TEXT("[%s] Failed to start dialogue execution: Already started"), *GetName());
		return false;
	}

	CurrentNodeId = NodeId;

	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		ReceiveDialoueExecutionBegin(CurrentNodeId, EntryPoint);
	}
	OnDialogueExecutionStarted.Broadcast(CurrentNodeId);
	ExecuteCurrentNode();

	return true;
}

void UDialogueExecutor::ExecuteCurrentNode()
{
	if (CurrentNodeId < 0 || bExecutionInProgress)
	{
		return;
	}
	bExecutionInProgress = true;

	OnNodeExecutionBegin.Broadcast(CurrentNodeId);	
	NodeExecutionBegin();
}

void UDialogueExecutor::FinishNodeExecution(int32 NextNodeId)
{	
	if (!bExecutionInProgress || bExecutionCleanupInProgress)
	{
		return;
	}		
	bExecutionCleanupInProgress = true;

	if (Dialogue)
	{	
		if (const FDialogueNode* Node = Dialogue->GetNodeMap().Find(CurrentNodeId))
		{
			UObject* Participant = GetParticipantFromNode(Node, true);
			if (Participant)
			{
				IDialogueParticipantInterface::Execute_OnNodeFinished(Participant, this, Dialogue, CurrentNodeId);
			}

			if (Node->Context)
			{
				Node->Context->OnNodeFinished(this);
			}
		}
	}

	NodeExecutionEnd();
	OnNodeExecutionEnd.Broadcast(CurrentNodeId);	
	bExecutionCleanupInProgress = false;

	bExecutionInProgress = false;

	if (NextNodeId >= 0 && MoveToNode(CurrentNodeId, NextNodeId, CurrentNodeId))
	{
		ExecuteCurrentNode();
	}
	else
	{
		if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
		{
			ReceiveDialoueExecutionEnd(CurrentNodeId);
		}
		OnDialogueExecutionFinished.Broadcast(CurrentNodeId);
	}
}

bool UDialogueExecutor::IsExecutionInProgress() const
{
	return CurrentNodeId >= 0;
}

int32 UDialogueExecutor::GetCurrentNodeId() const
{
	return CurrentNodeId;
}