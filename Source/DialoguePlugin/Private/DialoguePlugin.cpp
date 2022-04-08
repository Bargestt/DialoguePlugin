// Copyright Epic Games, Inc. All Rights Reserved.

#include "DialoguePlugin.h"
#include "Dialogue.h"
#include "DialogueExecutor.h"

DEFINE_LOG_CATEGORY(LogDialogue);
	
IMPLEMENT_MODULE(FDialoguePlugin, DialoguePlugin)


void UDialogueUtilityLibrary::CreateDialogueExecutor(TSubclassOf<UDialogueExecutorBase> Class, UObject* Owner, UDialogue* Dialogue, bool bDeferInitialization, UDialogueExecutorBase*& OutExecutor)
{
	OutExecutor = nullptr;
	if (Class && Owner)
	{
		OutExecutor = NewObject<UDialogueExecutorBase>(Owner, Class);
		if (OutExecutor)
		{
			OutExecutor->Dialogue = Dialogue;
			OutExecutor->HandleCreated();

			if (!bDeferInitialization)
			{
				OutExecutor->Initialize();
			}
		}
	}
}