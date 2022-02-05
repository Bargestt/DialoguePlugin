

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
