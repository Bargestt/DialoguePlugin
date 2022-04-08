// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.


#include "DialogueStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#include "Interfaces/IPluginManager.h"
#include "SlateOptMacros.h"


#define IMAGE_PLUGIN_BRUSH( RelativePath, ... ) FSlateImageBrush( FDialogueStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_BRUSH(RelativePath, ...) FSlateBoxBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_PLUGIN_BRUSH(RelativePath, ...) FSlateBoxBrush( FDialogueStyle::InContent( RelativePath, ".png" ), __VA_ARGS__)
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

FString FDialogueStyle::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("DialoguePlugin"))->GetContentDir();
	return (ContentDir / RelativePath) + Extension;
}

TSharedPtr< FSlateStyleSet > FDialogueStyle::StyleSet = nullptr;
TSharedPtr< class ISlateStyle > FDialogueStyle::Get() { return StyleSet; }

FName FDialogueStyle::GetStyleSetName()
{
	static FName PaperStyleName(TEXT("DialogueStyle"));
	return PaperStyleName;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FDialogueStyle::Initialize()
{
	// Const icon sizes
	const FVector2D Icon8x8(8.0f, 8.0f);
	const FVector2D Icon10x10(10.0f, 10.0f);
	const FVector2D Icon12x12(12.0f, 12.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}
	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	//////////////////////////////////////////////////////////////////////////
	// Register styles
	// 
	
	StyleSet->Set("DialogueEditor.Graph.DialogueNode.Index.Default", FSlateColor(FLinearColor(0.3f, 0.3f, 0.3f)));
	StyleSet->Set("DialogueEditor.Graph.DialogueNode.Index.Selected", FSlateColor(FLinearColor(0.0f, 0.25f, 0.5f)));

	StyleSet->Set("DialogueEditor.Graph.DialogueNode.Condition", new IMAGE_PLUGIN_BRUSH("Icons/DialogueNode_Condition", Icon16x16));
	StyleSet->Set("DialogueEditor.Graph.DialogueNode.Event", new IMAGE_PLUGIN_BRUSH("Icons/DialogueNode_Event", Icon16x16));
	StyleSet->Set("DialogueEditor.Graph.DialogueNode.Sound", new IMAGE_PLUGIN_BRUSH("Icons/DialogueNode_Sound", Icon16x16));

	StyleSet->Set("DialogueEditor.Graph.DialogueNode", new BOX_PLUGIN_BRUSH("Icons/DialogueNode_Body", FMargin(16.f / 64.f, 25.f / 64.f, 16.f / 64.f, 16.f / 64.f)));
	StyleSet->Set("DialogueEditor.Graph.DialogueNode.Border", new BOX_PLUGIN_BRUSH("Icons/DialogueNode_Border", FMargin(16.f / 64.f, 25.f / 64.f, 16.f / 64.f, 16.f / 64.f)));

	StyleSet->Set("DialogueEditor.Graph.DialogueEntryDefault", new BOX_PLUGIN_BRUSH("Icons/DialogueNode_EntryDefault", FMargin(16.f / 64.f, 25.f / 64.f, 16.f / 64.f, 16.f / 64.f)));


	StyleSet->Set("DialogueEditor.Graph.DialogueNode.DebugBorder", new BOX_PLUGIN_BRUSH("Icons/DialogueNode_DebugBorder", FMargin(16.f / 64.f, 16.f / 64.f, 16.f / 64.f, 16.f / 64.f)));

		
	StyleSet->Set("DialogueEditorCommands.AddSnippet", new IMAGE_PLUGIN_BRUSH("Icons/Icon_Apply_Snippet", Icon40x40));
	StyleSet->Set("DialogueEditorCommands.AddSnippet.Small", new IMAGE_PLUGIN_BRUSH("Icons/Icon_Apply_Snippet", Icon20x20));

	StyleSet->Set("DialogueEditorCommands.RefreshSnippets", new IMAGE_PLUGIN_BRUSH("Icons/Icon_refresh_Snippets", Icon40x40));
	StyleSet->Set("DialogueEditorCommands.RefreshSnippets.Small", new IMAGE_PLUGIN_BRUSH("Icons/Icon_refresh_Snippets", Icon20x20));

	StyleSet->Set("DialogueDebuggerCommands.StepBack", new IMAGE_PLUGIN_BRUSH("Icons/Icon_StepBackward", Icon40x40));
	StyleSet->Set("DialogueDebuggerCommands.StepForward", new IMAGE_PLUGIN_BRUSH("Icons/Icon_StepForward", Icon40x40));
	StyleSet->Set("DialogueDebuggerCommands.StepToBegin", new IMAGE_PLUGIN_BRUSH("Icons/Inco_ToBegin", Icon40x40));
	StyleSet->Set("DialogueDebuggerCommands.StepToEnd", new IMAGE_PLUGIN_BRUSH("Icons/Icon_ToEnd", Icon40x40));

	//////////////////////////////////////////////////////////////////////////

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
};

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef IMAGE_PLUGIN_BRUSH
#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef DEFAULT_FONT

void FDialogueStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}
