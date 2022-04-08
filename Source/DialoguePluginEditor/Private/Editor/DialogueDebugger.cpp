


#include "DialogueDebugger.h"
#include "DialogueExecutor.h"
#include "AssetEditor_Dialogue.h"
#include "Engine/Selection.h"
#include "EdGraph_Dialogue.h"
#include "EdGraphNode_DialogueBase.h"


#define LOCTEXT_NAMESPACE "DialogueDebugger"


FDialogueDebugger::FDialogueDebugger()
{
	FEditorDelegates::BeginPIE.AddRaw(this, &FDialogueDebugger::OnBeginPIE);
	FEditorDelegates::EndPIE.AddRaw(this, &FDialogueDebugger::OnEndPIE);

	ExecutionStep = -1;
}

FDialogueDebugger::~FDialogueDebugger()
{
	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
	USelection::SelectObjectEvent.RemoveAll(this);
	UDialogueExecutorBase::OnExecutorCreated.RemoveAll(this);

	ClearDebuggerState();
}

void FDialogueDebugger::Setup(UDialogue* Dialogue, TSharedRef<FAssetEditor_Dialogue> InEditorOwner)
{
	EditorOwner = InEditorOwner;
	Asset = InEditorOwner->GetAssetBeingEdited();
	KnownInstances.Reset();

	if (GEditor->bIsSimulatingInEditor || GEditor->PlayWorld)
	{	
		for (TObjectIterator<UDialogueExecutorBase> It(RF_ClassDefaultObject, true, EInternalObjectFlags::PendingKill); It; ++It)
		{
			class UWorld* World = It->GetWorld();
			if (World && World->IsPlayInEditor())
			{
				KnownInstances.Add(*It);
			}
		}
		OnBeginPIE(GEditor->bIsSimulatingInEditor);

		Refresh();
	}
}


void FDialogueDebugger::OnBeginPIE(const bool bIsSimulating)
{
	bIsPIEActive = true;
	if (EditorOwner.IsValid())
	{
		TSharedPtr<FAssetEditor_Dialogue> EditorOwnerPtr = EditorOwner.Pin();
		EditorOwnerPtr->RegenerateMenusAndToolbars();
	}

	USelection::SelectObjectEvent.RemoveAll(this);
	USelection::SelectObjectEvent.AddRaw(this, &FDialogueDebugger::OnObjectSelected);

	UDialogueExecutorBase::OnExecutorCreated.AddRaw(this, &FDialogueDebugger::OnExecutorCreated);
}

void FDialogueDebugger::OnEndPIE(const bool bIsSimulating)
{
	bIsPIEActive = false;
	if (EditorOwner.IsValid())
	{
		EditorOwner.Pin()->RegenerateMenusAndToolbars();
	}

	ClearDebuggerState();
}

void FDialogueDebugger::OnObjectSelected(UObject* Object)
{
	if (Object && Object->IsSelected())
	{
		TArray<UDialogueExecutorBase*> SubObjExecutors;
		ForEachObjectWithOuter(Object, [&SubObjExecutors](UObject* InSubObject)
		{
			if (UDialogueExecutorBase* Executor = Cast<UDialogueExecutorBase>(InSubObject))
			{
				SubObjExecutors.Add(Executor);
			}
		}, true, RF_NoFlags, EInternalObjectFlags::PendingKill);


		for (UDialogueExecutorBase* Executor : SubObjExecutors)
		{
			if (Executor->GetDialogue() == Asset)
			{
				ClearDebuggerState();
				ExecutorInstance = Executor;
				InitDebuggerState();
				
				Refresh();
				break;
			}						
		}
	}
}

void FDialogueDebugger::OnExecutorCreated(const UDialogueExecutorBase& Executor)
{
	TWeakObjectPtr<UDialogueExecutorBase> KnownInstance = const_cast<UDialogueExecutorBase*>(&Executor);
	KnownInstances.AddUnique(KnownInstance);

	if (!ExecutorInstance.IsValid() && Executor.GetDialogue() == Asset)
	{
		ClearDebuggerState();
		ExecutorInstance = KnownInstance;
		InitDebuggerState();

		Refresh();
	}
}

bool FDialogueDebugger::IsDebuggerReady() const
{
	return bIsPIEActive;
}

FString FDialogueDebugger::GetDebuggedInstanceDesc() const
{
	UDialogueExecutorBase* Executor = ExecutorInstance.Get();
	return Executor ? DescribeInstance(*Executor) : NSLOCTEXT("BlueprintEditor", "DebugActorNothingSelected", "No debug object selected").ToString();
}

FString FDialogueDebugger::DescribeInstance(UDialogueExecutorBase& InstanceToDescribe) const
{
	FString InstanceName = InstanceToDescribe.GetName();

	FString OuterName = TEXT("");
	UObject* Outer = InstanceToDescribe.GetOuter();
	if (AActor* Actor = Cast<AActor>(Outer))
	{
		OuterName = Actor->GetActorLabel();
	}
	else if (UActorComponent* Comp = Cast<UActorComponent>(Outer))
	{
		AActor* Owner = Comp->GetOwner();
		OuterName = (Owner ? Owner->GetActorLabel() : TEXT("None")) + TEXT(".") + Comp->GetName();
	}
	else if (Outer)
	{
		OuterName = Outer->GetName();
	}

	if (!OuterName.IsEmpty())
	{				
		FString IsSelected = (Outer && Outer->IsSelected()) ? TEXT("->") : TEXT("   ");		
		InstanceName = FString::Printf(TEXT("%s %s( %s )"), *InstanceName, *IsSelected, *OuterName);
	}

	return InstanceName;
}

void FDialogueDebugger::GetMatchingInstances(TArray<UDialogueExecutorBase*>& MatchingInstances)
{	
	for (int32 Index = KnownInstances.Num() - 1; Index >= 0; Index--)
	{
		UDialogueExecutorBase* TestInstance = KnownInstances[Index].Get();
		if (TestInstance == NULL)
		{
			KnownInstances.RemoveAt(Index);
			continue;
		}

		if (TestInstance->GetDialogue() == Asset)
		{
			MatchingInstances.Add(TestInstance);
		}		
	}
}

void FDialogueDebugger::OnInstanceSelectedInDropdown(UDialogueExecutorBase* SelectedInstance)
{
	if (SelectedInstance)
	{
		ClearDebuggerState();
		ExecutorInstance = SelectedInstance;
		InitDebuggerState();

		Refresh();
	}
}

void FDialogueDebugger::ClearDebuggerState()
{
	ActiveNodes.Empty();
	CompletedNodes.Empty();
	ActiveNodeAllowedTransitions.Empty();
	ActiveNodeDeniedTransitions.Empty();

	if (ExecutorInstance.IsValid())
	{
		ExecutorInstance->OnLogChanged.RemoveAll(this);
	}

	UpdateNodeStates();
}

void FDialogueDebugger::InitDebuggerState()
{
	if (ExecutorInstance.IsValid())
	{
		ExecutorInstance->OnLogChanged.AddRaw(this, &FDialogueDebugger::Refresh);
	}
}

void FDialogueDebugger::Refresh()
{
	if (!IsDebuggerReady())
	{
		return;
	}

	if (!ExecutorInstance.IsValid() || !Asset)
	{
		return;
	}	

	typedef UDialogueExecutorBase::FDialogueExecutionStep FDebuggerStep;
	typedef UDialogueExecutorBase::FDialogueExecutionStep::EExecutionAction EExecutionAction;

	const TArray<FDebuggerStep>& Log = ExecutorInstance->ExecutionLog;

	StepIndices.Empty();
	for (int32 Index = 0; Index < Log.Num(); Index++)
	{
		const FDebuggerStep& Step = Log[Index];
		if (Step.Action == EExecutionAction::Active)
		{
			TArray<int32> SubSteps;

			int32 SubStep = Index;
			while (SubStep + 1 < Log.Num())
			{
				SubStep++;
				if (Log[SubStep].Action == EExecutionAction::Active)
				{
					SubStep--;
					if (Log[SubStep].Action == EExecutionAction::Finished && Log[SubStep].NodeId == Step.NodeId)
					{
						SubStep--;
					}
					break;
				}

				if (Log[SubStep].Action == EExecutionAction::Finished && Log[SubStep].NodeId != Step.NodeId)
				{
					SubSteps.Add(SubStep);
				}
			}

			StepIndices.Add(SubStep);

			StepIndices.Append(SubSteps);
		}
	}

	ExecutionStep = -1;
	SetStep(StepIndices.Num() - 1);
}

void FDialogueDebugger::SetStep(int32 NewStep)
{	
	int32 NewExecutionStep = FMath::Clamp(NewStep, 0, FMath::Max(0, StepIndices.Num() - 1));

	if (NewExecutionStep == ExecutionStep)
	{
		return;
	}	
	ExecutionStep = NewExecutionStep;

	if (!IsDebuggerReady())
	{
		return;
	}

	if (!ExecutorInstance.IsValid() || !Asset)
	{
		return;
	}

	typedef UDialogueExecutorBase::FDialogueExecutionStep FDebuggerStep;
	typedef UDialogueExecutorBase::FDialogueExecutionStep::EExecutionAction EExecutionAction;

	const TArray<FDebuggerStep>& Log = ExecutorInstance->ExecutionLog;

	ActiveNodes.Empty();
	CompletedNodes.Empty();
	ActiveNodeAllowedTransitions.Empty();
	ActiveNodeDeniedTransitions.Empty();

	NodeConditionStates.Empty();
		

	int32 WalkToStep = StepIndices.IsValidIndex(ExecutionStep) ? StepIndices[ExecutionStep] : Log.Num() - 1;

	for (int32 StepIndex = 0; StepIndex <= WalkToStep; StepIndex++)
	{
		const FDebuggerStep& Step = Log[StepIndex];

		if (Step.NodeId < 0)
		{
			continue;
		}

		switch (Step.Action)
		{
		case FDebuggerStep::Active:
		{
			ActiveNodes.Add(Step.NodeId);

			ActiveNodeAllowedTransitions.Remove(Step.NodeId);
			ActiveNodeDeniedTransitions.Remove(Step.NodeId);
		}
			break;
		case FDebuggerStep::Finished:
		{
			ActiveNodes.Remove(Step.NodeId);
			CompletedNodes.Add(Step.NodeId);

			ActiveNodeAllowedTransitions.Remove(Step.NodeId);
			ActiveNodeDeniedTransitions.Remove(Step.NodeId);
		}
			break;
		case FDebuggerStep::EntryUnknown:
		{
			AllowedNodes.Remove(Step.NodeId);
			DeniedNodes.Remove(Step.NodeId);
		}
			break;
		case FDebuggerStep::EntryAllowed:
		{
			AllowedNodes.Add(Step.NodeId);
			DeniedNodes.Remove(Step.NodeId);
		}
			break;
		case FDebuggerStep::EntryDenied:
		{
			DeniedNodes.Add(Step.NodeId);
			AllowedNodes.Remove(Step.NodeId);
		}
			break;
		case FDebuggerStep::None:
		default:
			break;
		}
	}

	// Show transition states for active nodes only
	for (int32 ActiveNode : ActiveNodes)
	{
		TArray<int32> ChildrenIds = Asset->GetChildrenIds(ActiveNode);
		for (int32 ChildId : ChildrenIds)
		{
			bool bIsAllowed = AllowedNodes.Contains(ChildId);
			bool bIsDenied = DeniedNodes.Contains(ChildId);
			NodeConditionStates.Add(ChildId, (bIsAllowed && !bIsDenied) || (!bIsAllowed && !bIsDenied));
		}
	}
	
	UpdateNodeStates();
}

void FDialogueDebugger::UpdateNodeStates()
{	
	UEdGraph* Graph = Asset->EdGraph;
	if (!Graph)
	{
		return;
	}

	for (int32 Index = 0; Index < Graph->Nodes.Num(); Index++)
	{
		UEdGraphNode_DialogueBase* Node = Cast<UEdGraphNode_DialogueBase>(Graph->Nodes[Index]);

		if (Node)
		{	
			Node->bDebuggerMark_Active = false;
			Node->bDebuggerMark_Finished = false;
			Node->bDebuggerMark_EntryDenied = false;
			Node->bDebuggerMark_EntryAllowed = false;

			int32 NodeId = Node->GetNodeId();
			if (NodeId >= 0)
			{
				Node->bDebuggerMark_Active = ActiveNodes.Contains(NodeId);
				Node->bDebuggerMark_Finished = CompletedNodes.Contains(NodeId);

				const bool* ConditionStatePtr = NodeConditionStates.Find(NodeId);
				if (ConditionStatePtr)
				{
					Node->bDebuggerMark_EntryDenied = *ConditionStatePtr == false;
					Node->bDebuggerMark_EntryAllowed = *ConditionStatePtr == true;
				}
			}
		}
	}
}

void FDialogueDebugger::StepBack()
{
	SetStep(ExecutionStep - 1);
}

bool FDialogueDebugger::CanStepBack()
{
	return ExecutionStep > 0;
}

void FDialogueDebugger::StepForward()
{
	SetStep(ExecutionStep + 1);
}

bool FDialogueDebugger::CanStepForward()
{
	return ExecutionStep < StepIndices.Num() - 1;
}

void FDialogueDebugger::StepToBegin()
{
	SetStep(0);
}

bool FDialogueDebugger::CanStepToBegin()
{
	return CanStepBack();
}

void FDialogueDebugger::StepToEnd()
{
	SetStep(StepIndices.Num() - 1);
}

bool FDialogueDebugger::CanStepToEnd()
{
	return CanStepForward();
}

#undef LOCTEXT_NAMESPACE
