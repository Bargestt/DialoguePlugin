


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
	for (UDialogueCondition* Condition : Conditions)
	{
		if (Condition)
		{
			bResult = bResult && Condition->CheckCondition(WorldContext);
			if (!bResult)
			{
				break;
			}
		}
	}

	return bResult;
}

bool UDialogueCondition_OR::IsConditionMet(UObject* WorldContext) const
{
	bool bResult = true;
	for (UDialogueCondition* Condition : Conditions)
	{
		if (Condition)
		{
			bResult = bResult || Condition->CheckCondition(WorldContext);
			if (bResult)
			{
				break;
			}
		}
	}

	return bResult;
}
