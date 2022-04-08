


#include "DialogueCondition.h"
#include "Dialogue.h"

bool UDialogueCondition::CheckCondition(UObject* WorldContext)
{
	bool Result = IsConditionMet(WorldContext);

	if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
	{
		Result = Result && BP_IsConditionMet(WorldContext);
	}

	return Result;
}

bool UDialogueCondition::BP_IsConditionMet_Implementation(UObject* WorldContext) const
{
	return true;
}

UDialogue* UDialogueCondition::GetDialogue() const
{
	return GetTypedOuter<UDialogue>();
}


bool UDialogueCondition_AND::IsConditionMet(UObject* WorldContext) const
{
	bool bResult = true;

	for (int32 Index = 0; Index < Conditions.Num() && bResult; Index++)
	{
		if (Conditions[Index])
		{
			bResult = bResult && Conditions[Index]->CheckCondition(WorldContext);
		}
	}

	return bResult;
}

bool UDialogueCondition_OR::IsConditionMet(UObject* WorldContext) const
{
	bool bResult = false;

	for (int32 Index = 0; Index < Conditions.Num() && !bResult; Index++)
	{
		if (Conditions[Index])
		{
			bResult = bResult || Conditions[Index]->CheckCondition(WorldContext);
		}		
	}

	return bResult;
}

bool UDialogueCondition_Equality::IsConditionMet(UObject* WorldContext) const
{
	bool bResult = (A && A->CheckCondition(WorldContext)) == (B && B->CheckCondition(WorldContext));	
	return bCheckEqual ? bResult : !bResult;
}
