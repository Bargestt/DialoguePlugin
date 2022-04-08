// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



#include "DialogueExecutor.h"
#include "DialoguePlugin.h"
#include "DialogueCondition.h"
#include "Dialogue.h"
#include "DialogueContext.h"
#include "DialogueParticipantInterface.h"
#include "DialogueEvent.h"

#if WITH_EDITOR
#include <Logging/MessageLog.h>
#include <Logging/TokenizedMessage.h>
#include <Misc/UObjectToken.h>
#endif // WITH_EDITOR

#define LOCTEXT_NAMESPACE "DialogueExecutor"

UDialogueExecutorBase::FOnExecutorCreated UDialogueExecutorBase::OnExecutorCreated;

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


UObject* UDialogueExecutorBase::ResolveParticipant(const FDialogueParticipant& Participant) const
{
	if (Participant.Name != NAME_None)
	{
		return Participants.FindRef(Participant.Name);
	}

	return Participant.Object;
}	

void UDialogueExecutorBase::SetDialogue(UDialogue* NewDialogue)
{
	if (NewDialogue != Dialogue)
	{
		Dialogue = NewDialogue;
		DIALOGUE_LOG_CLEAR();
	}		
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
		if (const FDialogueNode* Node = Dialogue->GetNodeMap().Find(NodeId))
		{
			return ResolveParticipant(Node->Participant);
		}
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

void UDialogueExecutorBase::SetParticipantMap(const TMap<FName, UObject*>& NewParticipants, bool bOverrideExisting /*= true*/)
{
	for (const auto& Pair : NewParticipants)
	{
		SetParticipant(Pair.Key, Pair.Value);
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

		bool bCanEnter = Node->CheckCondition(this);
		DIALOGUE_LOG_ADD(FDialogueExecutionStep(NodeId, bCanEnter ? FDialogueExecutionStep::EntryAllowed : FDialogueExecutionStep::EntryDenied));

		return bCanEnter;
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
				Child->CheckCondition(this);


			DIALOGUE_LOG_ADD(FDialogueExecutionStep(ChildId, bCanEnterChild ? FDialogueExecutionStep::EntryAllowed : FDialogueExecutionStep::EntryDenied));
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

	if (bTransitionInProgress)
	{
#if WITH_EDITOR
		FMessageLog("PIE").Error()
			->AddToken(FTextToken::Create(LOCTEXT("Blueprint", "Blueprint:")))
			->AddToken(GetOuter() ? FUObjectToken::Create(GetOuter()->GetClass(), GetOuter()->GetClass()->GetDisplayNameText()) : FUObjectToken::Create(nullptr))
			->AddToken(FTextToken::Create(LOCTEXT("Executor", " Executor:")))
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT("MoveToNodeError", "Recursive MoveToNode calls are prohibited")));

#endif //WITH_EDITOR
		return false;
	}
	bTransitionInProgress = true;

	
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

	bTransitionInProgress = false;
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
		Participant = Node ? ResolveParticipant(Node->Participant) : nullptr;
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
#if WITH_EDITOR
			FMessageLog("PIE").Error()
				->AddToken(FTextToken::Create(LOCTEXT("Executor", "Executor:")))
				->AddToken(FUObjectToken::Create(this))
				->AddToken(FTextToken::Create(FText::FormatOrdered(LOCTEXT("FormatError_InvalidArg", " Invalid argument {0}. Argument must be in format {TargetName.FunctionName}"), FText::FromString(ArgumentName))));
			
#endif // WITH_EDITOR
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
#if WITH_EDITOR
			FMessageLog("PIE").Error()
				->AddToken(FTextToken::Create(LOCTEXT("Executor", "Executor:")))
				->AddToken(FUObjectToken::Create(this))
				->AddToken(FTextToken::Create(FText::FormatOrdered(LOCTEXT("FormatError_NoTarget", " Target '{0}' not found"), FText::FromString(ArgumentName))))
				->AddToken(FTextToken::Create(LOCTEXT("FormatError_AllowedTargets", "Allowed targets: Executor, Dialogue, Participant, Context and all ParticipantKeys")));
			
#endif // WITH_EDITOR
			continue;
		}
			
		UFunction* Func = TargetObject->FindFunction(FName(*FuncNamePart));
		if (!Func)
		{
#if WITH_EDITOR
			FMessageLog("PIE").Error()
				->AddToken(FTextToken::Create(LOCTEXT("Executor", "Executor:")))
				->AddToken(FUObjectToken::Create(this))
				->AddToken(FTextToken::Create(FText::FormatOrdered(LOCTEXT("FormatError_NoFunction", " Function '{0}' not found in "), FText::FromString(FuncNamePart))))
				->AddToken(FUObjectToken::Create(TargetObject));
#endif // WITH_EDITOR
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
		
		bool bFunctionValid = true;

		if (bHasInputArguments)
		{
			bFunctionValid = false;
#if WITH_EDITOR
			FMessageLog("PIE").Error()
				->AddToken(FTextToken::Create(LOCTEXT("Executor", "Executor:")))
				->AddToken(FUObjectToken::Create(this))
				->AddToken(FTextToken::Create(LOCTEXT("FormatError_FuncError", " Function")))
				->AddToken(FUObjectToken::Create(Func))
				->AddToken(FTextToken::Create(LOCTEXT("FormatError_HasInput", " must have no input")));	
#endif // WITH_EDITOR
		}
		
		if (ReturnArguments != 1)
		{
			bFunctionValid = false;
#if WITH_EDITOR
			FMessageLog("PIE").Error()
				->AddToken(FTextToken::Create(LOCTEXT("Executor", "Executor:")))
				->AddToken(FUObjectToken::Create(this))
				->AddToken(FTextToken::Create(LOCTEXT("FormatError_FuncError", " Function")))
				->AddToken(FUObjectToken::Create(Func))
				->AddToken(FTextToken::Create(LOCTEXT("FormatError_NoOutput", " must have one output")));
#endif // WITH_EDITOR
		}

		if (ReturnType.Type == FPropertyTypeChecker::EType::None)
		{
			bFunctionValid = false;
#if WITH_EDITOR
			FMessageLog("PIE").Error()
				->AddToken(FTextToken::Create(LOCTEXT("Executor", "Executor:")))
				->AddToken(FUObjectToken::Create(this))
				->AddToken(FTextToken::Create(LOCTEXT("FormatError_FuncError", " Function")))
				->AddToken(FUObjectToken::Create(Func))
				->AddToken(FTextToken::Create(LOCTEXT("FormatError_WrongOutput", " has unrecognized output.")))
				->AddToken(FTextToken::Create(LOCTEXT("FormatError_AllowedTypes", "Allowed types: Text, String, Name, Int32, Float")));
#endif // WITH_EDITOR
		}

		if (!bFunctionValid)
		{
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
	if (!bWasCreated)
	{
		bWasCreated = true;
		OnExecutorCreated.Broadcast(*this);
	}
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
			UObject* Participant = ResolveParticipant(Node->Participant);
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
			UObject* Participant = ResolveParticipant(Node->Participant);
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



void UDialogueExecutorBase::HandleNodeExecutionBegin(int32 NodeId)
{
	DIALOGUE_LOG_ADD(FDialogueExecutionStep(NodeId, FDialogueExecutionStep::Active));
	OnNodeExecutionBegin.Broadcast(NodeId);
}

void UDialogueExecutorBase::HandleNodeExecutionEnd(int32 NodeId)
{
	if (!Dialogue)
	{	
		return;
	}

	if (const FDialogueNode* Node = Dialogue->GetNodeMap().Find(NodeId))
	{
		UObject* Participant = ResolveParticipant(Node->Participant);
		if (Participant)
		{
			IDialogueParticipantInterface::Execute_OnNodeFinished(Participant, this, Dialogue, NodeId);
		}

		if (Node->Context)
		{
			Node->Context->OnNodeFinished(this);
		}
	}

	DIALOGUE_LOG_ADD(FDialogueExecutionStep(NodeId, FDialogueExecutionStep::Finished));
	OnNodeExecutionEnd.Broadcast(NodeId);
}




/*--------------------------------------------
 	UDialogueExecutor
 *--------------------------------------------*/

UDialogueExecutor::UDialogueExecutor()
{
	CurrentNodeId = -1;
}


void UDialogueExecutor::SetDialogue(UDialogue* NewDialogue)
{
	if (IsExecutionInProgress())
	{
#if WITH_EDITOR
		FMessageLog("PIE").Error()
			->AddToken(FTextToken::Create(LOCTEXT("Blueprint", "Blueprint:")))
			->AddToken(GetOuter() ? FUObjectToken::Create(GetOuter()->GetClass(), GetOuter()->GetClass()->GetDisplayNameText()) : FUObjectToken::Create(nullptr))
			->AddToken(FTextToken::Create(LOCTEXT("Executor", " Executor:")))
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT("SetDialogueFailed", " Failed to set new dialogue: Execution in progress")));
#endif // WITH_EDITOR
		return;
	}
	Super::SetDialogue(NewDialogue);
}
 
bool UDialogueExecutor::BeginExecution(FName EntryPoint)
{
	return BeginExecution_Internal(Dialogue ? Dialogue->GetEntryId(EntryPoint) : INDEX_NONE, EntryPoint);
}

bool UDialogueExecutor::BeginExecutionAtNode(int32 NodeId)
{
	return BeginExecution_Internal(NodeId, NAME_None);
}

bool UDialogueExecutor::BeginExecution_Internal(int32 NodeId, FName EntryPoint)
{
	if (!WasInitialized())
	{
		Initialize();
	}

	if (!Dialogue)
	{
#if WITH_EDITOR
		FMessageLog("PIE").Error()
			->AddToken(FTextToken::Create(LOCTEXT("Blueprint", "Blueprint:")))
			->AddToken(GetOuter() ? FUObjectToken::Create(GetOuter()->GetClass(), GetOuter()->GetClass()->GetDisplayNameText()) : FUObjectToken::Create(nullptr))
			->AddToken(FTextToken::Create(LOCTEXT("Executor", " Executor:")))
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT("ExecutionStartError"," Failed to start execution:")))
			->AddToken(FTextToken::Create(LOCTEXT("ExecutionError_NoAsset", "No dialogue")));
#endif // WITH_EDITOR
		return false;
	}
	if (NodeId < 0)
	{		
#if WITH_EDITOR
		FMessageLog("PIE").Error()
			->AddToken(FTextToken::Create(LOCTEXT("Blueprint", "Blueprint:")))
			->AddToken(GetOuter() ? FUObjectToken::Create(GetOuter()->GetClass(), GetOuter()->GetClass()->GetDisplayNameText()) : FUObjectToken::Create(nullptr))
			->AddToken(FTextToken::Create(LOCTEXT("Executor", " Executor:")))
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT("ExecutionStartError", " Failed to start execution:")))
			->AddToken(FTextToken::Create(LOCTEXT("ExecutionError_InvalidNode", "Invalid node")));
#endif // WITH_EDITOR
		return false;
	}
	if (bNodeExecutionCleanupInProgress || IsExecutionInProgress())
	{
#if WITH_EDITOR
		FMessageLog("PIE").Warning()
			->AddToken(FTextToken::Create(LOCTEXT("Blueprint", "Blueprint:")))
			->AddToken(GetOuter() ? FUObjectToken::Create(GetOuter()->GetClass(), GetOuter()->GetClass()->GetDisplayNameText()) : FUObjectToken::Create(nullptr))
			->AddToken(FTextToken::Create(LOCTEXT("Executor", " Executor:")))
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT("ExecutionStartError", " Failed to start execution:")))
			->AddToken(FTextToken::Create(LOCTEXT("ExecutionError_AlreadyStarted", "Execution in progress")));
#endif // WITH_EDITOR
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
	if (CurrentNodeId < 0 || bNodeExecutionInProgress)
	{
		return;
	}
	bNodeExecutionInProgress = true;

	HandleNodeExecutionBegin(CurrentNodeId);
	NodeExecutionBegin();
}

void UDialogueExecutor::FinishNodeExecution(int32 NextNodeId)
{	
	if (!bNodeExecutionInProgress)
	{
		return;
	}		

	if (bNodeExecutionCleanupInProgress)
	{
#if WITH_EDITOR
		FMessageLog("PIE").Warning()
			->AddToken(FTextToken::Create(LOCTEXT("Blueprint", "Blueprint:")))
			->AddToken(GetOuter() ? FUObjectToken::Create(GetOuter()->GetClass(), GetOuter()->GetClass()->GetDisplayNameText()) : FUObjectToken::Create(nullptr))
			->AddToken(FTextToken::Create(LOCTEXT("Executor", " Executor:")))
			->AddToken(FUObjectToken::Create(this))
			->AddToken(FTextToken::Create(LOCTEXT("ExecutionFinishError", "Failed to finish execution")))
			->AddToken(FTextToken::Create(LOCTEXT("ExecutionError_Recusrsive", "Recursive call")));
#endif // WITH_EDITOR
		return;
	}

	bNodeExecutionCleanupInProgress = true;
	
	NodeExecutionEnd();
	HandleNodeExecutionEnd(CurrentNodeId);
	bNodeExecutionCleanupInProgress = false;

	bNodeExecutionInProgress = false;

	if (MoveToNode(CurrentNodeId, NextNodeId, CurrentNodeId))
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

void UDialogueExecutor::StopExecution()
{
	if (IsExecutionInProgress())
	{
		FinishNodeExecution(INDEX_NONE);
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

#undef  LOCTEXT_NAMESPACE 