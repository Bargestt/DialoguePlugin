// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueContext.h"
#include "Dialogue.h"

int32 UDialogueNodeContext::GetNodeId() const
{
	return NodeId;
}

UDialogue* UDialogueNodeContext::GetDialogue() const
{
	return Cast<UDialogue>(GetOuter());
}
