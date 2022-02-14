#pragma once

#include "CoreMinimal.h"
#include "DialogueExecutor.h"

class UDialogue;
class FAssetEditor_Dialogue;
class UDialogueExecutorBase;


struct FDialogueDebuggerStepVerbosity 
{	
	typedef UDialogueExecutorBase::FDialogueExecutionStep FDebuggerStep;
	typedef UDialogueExecutorBase::FDialogueExecutionStep::EExecutionAction EExecutionAction;

	FName Name;
	FString Description;

	virtual ~FDialogueDebuggerStepVerbosity()
	{ }
	
	virtual bool CheckStepImportant(const TArray<FDebuggerStep>& Steps, int32 Step, int32& OutWriteStep, int32& OutAdvanceBy) const
	{
		return false;
	}
};

struct FDDSV_Full : public FDialogueDebuggerStepVerbosity
{
	FDDSV_Full()
	{
		Name = FName("Full");
		Description = TEXT("Maximally detailed verbosity");
	}

	virtual bool CheckStepImportant(const TArray<FDebuggerStep>& Steps, int32 Step, int32& OutWriteStep, int32& OutAdvanceBy) const override
	{
		OutWriteStep = Step;
		OutAdvanceBy = 1;
		return true;
	}
};


struct FDDSV_Activation : public FDialogueDebuggerStepVerbosity
{
	FDDSV_Activation()
	{
		Name = FName("Activation");
		Description = TEXT("Jump between steps where node was activated");
	}

	virtual bool CheckStepImportant(const TArray<FDebuggerStep>& Steps, int32 Step, int32& OutWriteStep, int32& OutAdvanceBy) const override
	{		
		if (Steps[Step].Action == EExecutionAction::Active)
		{
			// Walk ahead until node finished or another is activated
			int32 StopAt = Step + 1;
			while (StopAt < Steps.Num())
			{
				if (Steps[StopAt].Action == EExecutionAction::Active || Steps[StopAt].Action == EExecutionAction::Finished)
				{
					// Stop before activation or finish
					StopAt--;
					break;
				}
				StopAt++;
			}

			OutWriteStep = StopAt;
			OutAdvanceBy = FMath::Max(1, StopAt - Step);
			return true;
		}

		return false;
	}
};




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

	TArray<TSharedRef<FDialogueDebuggerStepVerbosity>> VerbosityLevels;
	TSharedPtr<FDialogueDebuggerStepVerbosity> CurrentVerbosity;


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

	void OnVerbosityLevelSelectedInDropdown(FName Name);

	TArray<TSharedRef<FDialogueDebuggerStepVerbosity>> GetVerbosityLevels() const;
	TSharedRef<FDialogueDebuggerStepVerbosity> GetCurrentVerbosity() const;



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
