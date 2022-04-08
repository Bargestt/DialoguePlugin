

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "DialogueStyle.h"



class FDialogueEditorCommands : public TCommands<FDialogueEditorCommands>
{
public:
	FDialogueEditorCommands()
		: TCommands<FDialogueEditorCommands>("DialogueEditorCommands", NSLOCTEXT("DialogueEditorCommands", "DialogueEditorCommands", "Dialogue Editor Commands"), NAME_None, FDialogueStyle::Get()->GetStyleSetName())
	{ }

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> RefreshNode;

	TSharedPtr<FUICommandInfo> AddSnippet;
	TSharedPtr<FUICommandInfo> RefreshSnippets;
};


class FDialogueDebuggerCommands : public TCommands<FDialogueDebuggerCommands>
{
public:
	FDialogueDebuggerCommands()
		: TCommands<FDialogueDebuggerCommands>("DialogueDebuggerCommands", NSLOCTEXT("DialogueDebuggerCommands", "DialogueDebuggerCommands", "Dialogue Debugger Commands"), NAME_None, FDialogueStyle::Get()->GetStyleSetName())
	{ }

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> StepBack;
	TSharedPtr<FUICommandInfo> StepForward;

	TSharedPtr<FUICommandInfo> StepToBegin;
	TSharedPtr<FUICommandInfo> StepToEnd;
};
