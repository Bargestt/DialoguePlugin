// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DialogueEvent.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType, editinlinenew, meta = (ShowWorldContextPin))
class DIALOGUEPLUGIN_API UDialogueEvent : public UObject
{
	GENERATED_BODY()
public:

	virtual bool CanExecute(UObject* WorldContext, UDialogue* Dialogue, int32 NodeId)
	{
		if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
		{
			return BP_CanExecute(WorldContext, Dialogue, NodeId);
		}
		return true;
	}

	virtual void ExecuteEvent(UObject* WorldContext, UDialogue* Dialogue, int32 NodeId)
	{
		if (GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !GetClass()->HasAnyClassFlags(CLASS_Native))
		{
			BP_ExecuteEvent(WorldContext, Dialogue, NodeId);
		}
	}

protected:
	UFUNCTION(BlueprintNativeEvent, Category = Dialogue, meta = (DisplayName = "CanExecute"))
	bool BP_CanExecute(UObject* WorldContext, UDialogue* Dialogue, int32 NodeId);
	bool BP_CanExecute_Implementation(UObject* WorldContext, UDialogue* Dialogue, int32 NodeId)
	{
		return true;
	}

	UFUNCTION(BlueprintNativeEvent, Category = Dialogue, meta = (DisplayName = "ExecuteEvent"))
	void BP_ExecuteEvent(UObject* WorldContext, UDialogue* Dialogue, int32 NodeId);
	void BP_ExecuteEvent_Implementation(UObject* WorldContext, UDialogue* Dialogue, int32 NodeId)
	{

	}

};
