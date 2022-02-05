// Fill out your copyright notice in the Description page of Project Settings.


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

	EntryColor = FLinearColor::Red;
	NodeColor = FLinearColor::White;
	NodeBodyColorSource = EColorSource::None;

	NodeBorderColor = FLinearColor::Transparent;
	NodeBorderColorSource = EColorSource::Both;

	MaxPreviewTextLines = 5;
	NodeMaxBodySize = 75.0f;
}
