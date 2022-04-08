// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



#include "EditorCommands_Dialogue.h"

//////////////////////////////////////////////////////////////////////////
// FDialogueEditorCommands

#define LOCTEXT_NAMESPACE "DialogueEditorCommands"

void FDialogueEditorCommands::RegisterCommands()
{
	UI_COMMAND(RefreshNode, "RefreshNode", "Refresh Node", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(AddSnippet, "AddSnippet", "Add Snippet", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RefreshSnippets, "RefreshSnippets", "Refresh Snippets", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE


#define LOCTEXT_NAMESPACE "DialogueDebuggerCommands"

void FDialogueDebuggerCommands::RegisterCommands()
{
	UI_COMMAND(StepBack, "StepBack", "Step Back", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(StepForward, "StepForward", "Step Forward", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(StepToBegin, "StepToBegin", "Step To Begin", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(StepToEnd, "StepToEnd", "Step To End", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE