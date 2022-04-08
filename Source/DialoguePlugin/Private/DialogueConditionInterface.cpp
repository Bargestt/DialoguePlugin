


#include "DialogueConditionInterface.h"
#include "Dialogue.h"


FString IDialogueConditionInterface::GetConditionTitle_Implementation(bool bFullTitle) const
{
	UObject* Object = _getUObject();
	return Object ?
#if WITH_EDITORONLY_DATA
		Object->GetClass()->GetDisplayNameText().ToString()
#else
		Object->GetClass()->GetName()
#endif
		: TEXT("");
}




bool UDialogueCondition_AND::IsConditionMet(UObject* WorldContext, UObject* Dialogue, int32 NodeId) const
{
	bool bResult = true;

	for (int32 Index = 0; Index < Conditions.Num() && bResult; Index++)
	{
		if (Conditions[Index].IsValid())
		{
			bResult = bResult && Conditions[Index].CheckCondition(WorldContext, Dialogue, NodeId);
		}
	}

	return bResult;
}

FString UDialogueCondition_AND::GetConditionTitle_Implementation(bool bFullTitle) const
{	
	const FString ConditionName = TEXT("<RichTextBlock.Bold>AND</>");
	const FString DefaultValue = TEXT("true");

	if (!bFullTitle)
	{
		return FString::Printf(TEXT("%s( %d )"), *ConditionName, Conditions.Num());
	}		

	TArray<FString> Lines;
	for (int32 Index = 0; Index < Conditions.Num(); Index++)
	{			
		Lines.Add(FString::Printf(TEXT("(%s)"), 
			Conditions[Index].IsValid() ? *IDialogueConditionInterface::GetTitleSafe(Conditions[Index].Condition, bFullTitle) : *DefaultValue));
	}
	if (Lines.Num() < 2)
	{
		Lines.Add(FString::Printf(TEXT("(%s)"), *DefaultValue));
	}
	if (Lines.Num() < 2)
	{
		Lines.Add(FString::Printf(TEXT("(%s)"), *DefaultValue));
	}
	
	return FString::Join(Lines, *ConditionName);
}

bool UDialogueCondition_OR::IsConditionMet(UObject* WorldContext, UObject* Dialogue, int32 NodeId) const
{
	bool bResult = false;

	for (int32 Index = 0; Index < Conditions.Num() && !bResult; Index++)
	{
		if (Conditions[Index].IsValid())
		{
			bResult = bResult || Conditions[Index].CheckCondition(WorldContext, Dialogue, NodeId);
		}		
	}

	return bResult;
}

FString UDialogueCondition_OR::GetConditionTitle_Implementation(bool bFullTitle) const
{
	const FString ConditionName = TEXT("<RichTextBlock.Bold>OR</>");
	const FString DefaultValue = TEXT("false");

	if (!bFullTitle)
	{
		return FString::Printf(TEXT("%s( %d )"), *ConditionName, Conditions.Num());
	}

	TArray<FString> Lines;
	for (int32 Index = 0; Index < Conditions.Num(); Index++)
	{
		Lines.Add(FString::Printf(TEXT("(%s)"),
			Conditions[Index].IsValid() ? *IDialogueConditionInterface::GetTitleSafe(Conditions[Index].Condition, bFullTitle) : *DefaultValue));
	}
	if (Lines.Num() < 2)
	{
		Lines.Add(FString::Printf(TEXT("(%s)"), *DefaultValue));
	}
	if (Lines.Num() < 2)
	{
		Lines.Add(FString::Printf(TEXT("(%s)"), *DefaultValue));
	}

	return FString::Join(Lines, *ConditionName);
}

bool UDialogueCondition_Equality::IsConditionMet(UObject* WorldContext, UObject* Dialogue, int32 NodeId) const
{
	bool bResult = (A.IsValid() && A.CheckCondition(WorldContext, Dialogue, NodeId)) == (B.IsValid() && B.CheckCondition(WorldContext, Dialogue, NodeId));
	return bCheckEqual ? bResult : !bResult;
}

FString UDialogueCondition_Equality::GetConditionTitle_Implementation(bool bFullTitle) const
{
	FString TitleOP = bCheckEqual ? TEXT("==") : TEXT("!=");
	FString TitleA = A.IsValid() ? IDialogueConditionInterface::GetTitleSafe(A.Condition, bFullTitle) : TEXT("false");
	FString TitleB = B.IsValid() ? IDialogueConditionInterface::GetTitleSafe(B.Condition, bFullTitle) : TEXT("false");

	if (bFullTitle)
	{
		return FString::Printf(TEXT("(%s) <RichTextBlock.Bold>%s</> (%s)"), *TitleA, *TitleOP, *TitleB);
	}
	return FString::Printf(TEXT("%s <RichTextBlock.Bold>%s</> %s"), *TitleA, *TitleOP, *TitleB);
}

