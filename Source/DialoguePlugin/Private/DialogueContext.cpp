// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



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
