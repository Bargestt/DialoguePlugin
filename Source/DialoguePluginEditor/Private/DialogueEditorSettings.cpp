// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.




#include "DialogueEditorSettings.h"



UDialogueEditorSettings::UDialogueEditorSettings()
{
	AssetCategory = NSLOCTEXT("DialogueEditorSettings", "AssetCategory", "Dialogue");
	DefaultAssetClass = UDialogue::StaticClass();
	bUniformContext = true;

	//NodeShortDescFormat = NSLOCTEXT("DialogueEditorSettings", "NodeShortDescFormat", "{ParticipantName}: {DialogueTextShort}");

	WireColor = FLinearColor::White;
	WireColor_Selected = FLinearColor::White;
	ReturnWireColor = FLinearColor::White;
	ReturnWireColor_Selected = FLinearColor::White;

	WireThickness = 1.5f;
	WireThicknessScale_Selected = 1.5f;
	ReturnWireThicknessScale = 1.0f;	

	PinSize = 10.0f;

	EntryColor = FLinearColor(0.4f, 0.00f, 0.01f, 1.0f);
	NodeColor = FLinearColor(0.06f, 0.06f, 0.06f, 1.0f);
	NodeBodyColorSource = EColorSource::None;

	NodeBorderColor = FLinearColor::Transparent;
	NodeBorderColorSource = EColorSource::Both;

	NodeIconScale = 1.0f;
	NodeIconHoverScale = 4.0f;

	MaxPreviewTextLines = 5;
	NodeMaxBodySize = 75.0f;

	DebuggerState_Active = FLinearColor(1.0f, 0.45f, 0.0f);
	DebuggerState_Completed = FLinearColor(0.3f, 0.0f, 0.9f);
	DebuggerState_EntryAllowed = FLinearColor::Green;
	DebuggerState_EntryDenied = FLinearColor::Red;
}
