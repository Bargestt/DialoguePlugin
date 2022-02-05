// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DialogueEvent.generated.h"

/**
 * 
 */
UCLASS(Abstract, Blueprintable, editinlinenew)
class DIALOGUEPLUGIN_API UDialogueEvent : public UObject
{
	GENERATED_BODY()
public:

	bool CanExecute(UObject* WorldContext, UDialogue* Dialogue, int32 NodeId)
	{
		return true;
	}

	void ExecuteEvent(UObject* WorldContext, UDialogue* Dialogue, int32 NodeId)
	{

	}
};
