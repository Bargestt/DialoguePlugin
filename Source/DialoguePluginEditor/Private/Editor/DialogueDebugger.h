

#pragma once

#include "CoreMinimal.h"

class UDialogue;
class FAssetEditor_Dialogue;
class UDialogueExecutorBase;



class FDialogueDebugger
{
private:
	/** owning editor */
	TWeakPtr<FAssetEditor_Dialogue> EditorOwner;

	/** asset for debugging */
	UDialogue* Asset;

	TWeakObjectPtr<UDialogueExecutorBase> ExecutorInstance;

	/** all known instances, cached for dropdown list */
	TArray<TWeakObjectPtr<UDialogueExecutorBase>> KnownInstances;

	uint32 bIsPIEActive : 1;


	/** -1 is last step */
	int32 ExecutionStep;
	TArray<int32> StepIndices;

	enum StepVerbosity
	{
		Activation,
		Finish,
		Full
	};
	StepVerbosity Verbosity;


	// Execution state
	TSet<int32> ActiveNodes;	
	TSet<int32> CompletedNodes;

	TSet<int32> AllowedNodes;
	TSet<int32> DeniedNodes;

	TMap<int32, bool> NodeConditionStates;

	TMap<int32, TArray<int32>> ActiveNodeAllowedTransitions;
	TMap<int32, TArray<int32>> ActiveNodeDeniedTransitions;

public:
	FDialogueDebugger();
	~FDialogueDebugger();
	void Setup(UDialogue* Dialogue, TSharedRef<FAssetEditor_Dialogue> InEditorOwner);

	void OnBeginPIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);

	void OnObjectSelected(UObject* Object);
	void OnExecutorCreated(const UDialogueExecutorBase& Executor);

	bool IsDebuggerReady() const;
	FString GetDebuggedInstanceDesc() const;
	FString DescribeInstance(UDialogueExecutorBase& InstanceToDescribe) const;
	void GetMatchingInstances(TArray<UDialogueExecutorBase*>& MatchingInstances);
	void OnInstanceSelectedInDropdown(UDialogueExecutorBase* SelectedInstance);


	void ClearDebuggerState();
	void InitDebuggerState();

	void Refresh();

	void SetStep(int32 NewStep);

	void UpdateNodeStates();


public:
	// Commands
	void StepBack();
	bool CanStepBack();

	void StepForward();
	bool CanStepForward();

	void StepToBegin();
	bool CanStepToBegin();

	void StepToEnd();
	bool CanStepToEnd();
};
