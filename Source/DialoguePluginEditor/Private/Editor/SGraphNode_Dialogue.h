// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UEdGraphNode_DialogueBase;


class SGraphNode_Dialogue : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_Dialogue) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphNode_DialogueBase* InNode);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;

	virtual void UpdateGraphNode() override;
	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty = true) override;
	virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const override;

	void UpdateCachedValues();


	const FSlateBrush* GetNodeBodyBrush() const override;

	FSlateColor GetDebugBorderColor() const;
	FMargin GetDebugBorderPadding() const;

	EVisibility GetContentVisibility() const;
	EVisibility GetContextVisibility() const;

	EVisibility GetIndexVisibility() const;
	FText GetIndexText() const;
	FSlateColor GetIndexColor() const;

	EVisibility GetIconVisibility() const;
	EVisibility GetConditionVisibility() const;
	EVisibility GetEventVisibility() const;
	EVisibility GetSoundVisibility() const;	


	FSlateColor GetBodyColor() const { return CachedBodyColor; }
	FSlateColor GetBorderColor() const { return CachedBorderColor; }
	FText GetBodyText() const { return CachedBodyText; }
	FText GetContextText() const { return CachedContextText; }
	const FSlateBrush* GetNodeIcon() const { return &CachedIconBrush; }


	
protected:
	int32 CacheRefreshID;

	FLinearColor CachedBodyColor;
	FLinearColor CachedBorderColor;
	FText CachedBodyText;
	FText CachedContextText;
	FSlateBrush CachedIconBrush;


	TSharedPtr<SHorizontalBox> OutputPinBox;

	float OverlayIconSize;
	float OverlayMainIconSize;

	TSharedPtr<SWidget> ExecutionWidget;
	TSharedPtr<SWidget> IconWidget;
	TSharedPtr<SWidget> ConditionWidget;
	TSharedPtr<SWidget> EventWidget;
	TSharedPtr<SWidget> SoundWidget;
	
	const class UDialogueEditorSettings* DialogueSettings;
};
