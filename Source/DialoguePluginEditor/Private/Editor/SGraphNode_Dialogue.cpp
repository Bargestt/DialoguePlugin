// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



#include "SGraphNode_Dialogue.h"
#include "EdGraphNode_DialogueBase.h"


#include "GraphEditorSettings.h"
#include "SCommentBubble.h"
#include "SLevelOfDetailBranchNode.h"
#include <SGraphPin.h>

#include "SlateOptMacros.h"
#include <IDocumentation.h>
#include <TutorialMetaData.h>
#include <Widgets/Notifications/SErrorText.h>
#include "EdGraphNode_DialogueEntry.h"
#include <SNodePanel.h>
#include <SGraphPanel.h>
#include "DialogueStyle.h"
#include "EdGraph_Dialogue.h"
#include "DialogueEditorSettings.h"
#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Widgets/Layout/SScrollBox.h>
#include "EdGraphNode_DialogueNode.h"
#include <Widgets/Layout/SConstraintCanvas.h>


#define LOCTEXT_NAMESPACE "SGraphNode_Dialogue"


class SGraphPin_Dialogue : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SGraphPin_Dialogue) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
	{
		this->SetCursor(EMouseCursor::Default);

		bShowLabel = true;

		GraphPinObj = InPin;
		check(GraphPinObj != NULL);

		const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
		check(Schema);

		SBorder::Construct(SBorder::FArguments()
			.BorderImage(this, &SGraphPin_Dialogue::GetPinBorder)
			.BorderBackgroundColor(this, &SGraphPin_Dialogue::GetPinColor)
			.OnMouseButtonDown(this, &SGraphPin_Dialogue::OnPinMouseDown)
			.Cursor(this, &SGraphPin_Dialogue::GetPinCursor)
			.Padding(FMargin(UDialogueEditorSettings::Get()->PinSize))
		);
	}
protected:
	//~ Begin SGraphPin Interface
	virtual FSlateColor GetPinColor() const override
	{
		return FSlateColor(IsHovered() ? FLinearColor::Yellow : FLinearColor::Transparent);
	}
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override
	{
		return SNew(STextBlock);
	}

	//~ End SGraphPin Interface

	const FSlateBrush* GetPinBorder() const
	{
		return FEditorStyle::GetBrush(TEXT("Graph.StateNode.Body"));
	}

};

/** Widget for overlaying an execution-order index onto a node */
class SExecutionIndex : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_RetVal(FSlateColor, FOnGetIndexColor);


	SLATE_BEGIN_ARGS(SExecutionIndex){}
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_EVENT(FOnGetIndexColor, OnGetIndexColor)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs )
	{	
		OnGetIndexColorEvent = InArgs._OnGetIndexColor;

		const FSlateBrush* IndexBrush = FEditorStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Index"));

		ChildSlot
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				// Add a dummy box here to make sure the widget doesnt get smaller than the brush
				SNew(SBox)
				.WidthOverride(IndexBrush->ImageSize.X)
				.HeightOverride(IndexBrush->ImageSize.Y)
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SBorder)
				.BorderImage(IndexBrush)
				.BorderBackgroundColor(this, &SExecutionIndex::GetColor)
				.Padding(FMargin(4.0f, 0.0f, 4.0f, 1.0f))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(InArgs._Text)
					.Font(FEditorStyle::GetFontStyle("BTEditor.Graph.BTNode.IndexText"))
				]
			]
		];
	}

	FSlateColor GetColor() const
	{
		if(OnGetIndexColorEvent.IsBound())
		{
			return OnGetIndexColorEvent.Execute();
		}

		return FSlateColor::UseForeground();
	}

private:
	FOnGetIndexColor OnGetIndexColorEvent;
};

class SOverlayIcon : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SOverlayIcon){}
		SLATE_ATTRIBUTE(const FSlateBrush*, Image)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, float Size, float InHoverSize = -1.0f)
	{
		DefaultSize = Size;		
		HoverSize = InHoverSize;

		ChildSlot
		[
			SNew(SConstraintCanvas)
			+ SConstraintCanvas::Slot()
			.Anchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f))
			.Alignment(FVector2D(0.5f))
			.Offset(TAttribute<FMargin>(this, &SOverlayIcon::GetImageSize))
			[
				SNew(SImage)
				.Image(InArgs._Image)
			]			
		];
	}	

	FMargin GetImageSize() const
	{
		float Size = (HoverSize > 0 && IsHovered()) ? HoverSize : DefaultSize;
		return FMargin(0.0f, 0.0f, Size, Size);
	}

protected:

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		if (GetVisibility() != EVisibility::Collapsed)
		{
			return FVector2D(DefaultSize);
		}
		return FVector2D::ZeroVector;
	}

protected:
	float HoverSize;
	float DefaultSize;
};


void SGraphNode_Dialogue::Construct(const FArguments& InArgs, UEdGraphNode_DialogueBase* InNode)
{
	GraphNode = InNode;	

	IsEditable = false;

	DialogueSettings = UDialogueEditorSettings::Get();

	OverlayIconSize = 24.0f;
	OverlayMainIconSize = 32.0f * DialogueSettings->NodeIconScale;
	float HoverSize = FMath::Max(OverlayMainIconSize, OverlayMainIconSize * DialogueSettings->NodeIconHoverScale);


	ExecutionWidget = 
		SNew(SExecutionIndex)
		.Visibility(this, &SGraphNode_Dialogue::GetIndexVisibility)
		.Text(this, &SGraphNode_Dialogue::GetIndexText)
		.OnGetIndexColor(this, &SGraphNode_Dialogue::GetIndexColor);
		
	IconWidget = SNew(SOverlayIcon, OverlayMainIconSize, HoverSize)
		.Image(this, &SGraphNode_Dialogue::GetNodeIcon)
		.Visibility(this, &SGraphNode_Dialogue::GetIconVisibility);

	SoundWidget = SNew(SOverlayIcon, OverlayIconSize)
		.Image(FDialogueStyle::Get()->GetBrush(TEXT("DialogueEditor.Graph.DialogueNode.Sound")))
		.Visibility(this, &SGraphNode_Dialogue::GetSoundVisibility);

	ConditionWidget = SNew(SOverlayIcon, OverlayIconSize)
		.Image(FDialogueStyle::Get()->GetBrush(TEXT("DialogueEditor.Graph.DialogueNode.Condition")))
		.Visibility(this, &SGraphNode_Dialogue::GetConditionVisibility);

	EventWidget = SNew(SOverlayIcon, OverlayIconSize)
		.Image(FDialogueStyle::Get()->GetBrush(TEXT("DialogueEditor.Graph.DialogueNode.Event")))
		.Visibility(this, &SGraphNode_Dialogue::GetEventVisibility);


	UpdateGraphNode();
}

void SGraphNode_Dialogue::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (GraphNode && GraphNode->GetSchema() && GraphNode->GetSchema()->IsCacheVisualizationOutOfDate(CacheRefreshID))
	{
		UpdateCachedValues();
		CacheRefreshID = GraphNode->GetSchema()->GetCurrentVisualizationCacheID();
	}
}

FReply SGraphNode_Dialogue::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = SGraphNode::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);

	RequestRename();
	if (!Reply.IsEventHandled())
	{		
		Reply = FReply::Handled();
	}
	return Reply;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SGraphNode_Dialogue::UpdateGraphNode()
{
	const FMargin NodePadding = FMargin(10.0f);

	InputPins.Empty();
	OutputPins.Empty();
	
	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	
	OutputPinBox.Reset();

	//
	//             ______________________
	//            |		  InputPin		 |
	//            +----------------------+
	//            |      TITLE AREA      |
	//            +----------------------+
	//            |		DialogueText	 |
	//            |						 |
	//            |		ContextLines	 |
	//            |						 |
	//            +----------------------+
	//            |		  OutputPin		 |
	//            |______________________|
	//
	

	TSharedPtr<SVerticalBox> MainVerticalBox;
	SetupErrorReporting();

	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);


	// Get node icon
	IconColor = FLinearColor::White;
	const FSlateBrush* IconBrush = nullptr;
	if (GraphNode != NULL && GraphNode->ShowPaletteIconOnNode())
	{
		IconBrush = GraphNode->GetIconAndTint(IconColor).GetOptionalIcon();
	}


	
	if (!SWidget::GetToolTip().IsValid())
	{
		TSharedRef<SToolTip> DefaultToolTip = IDocumentation::Get()->CreateToolTip( TAttribute< FText >( this, &SGraphNode_Dialogue::GetNodeTooltip ), NULL, GraphNode->GetDocumentationLink(), GraphNode->GetDocumentationExcerptName() );
		SetToolTip(DefaultToolTip);
	}

	FGraphNodeMetaData TagMeta(TEXT("Graphnode"));
	PopulateMetaTag(&TagMeta);

	TSharedPtr<SVerticalBox> InnerVerticalBox;
	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);


	InnerVerticalBox =
		SNew(SVerticalBox)
		// INPUT PIN AREA
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.MinDesiredHeight(NodePadding.Top)
			[
				SAssignNew(LeftNodeBox, SVerticalBox)
			]
		]
		// STATE NAME AREA
		+ SVerticalBox::Slot()
		.Padding(FMargin(NodePadding.Left, 0.0f, NodePadding.Right, 0.0f))
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			//.BorderImage(FEditorStyle::GetBrush("BTEditor.Graph.BTNode.Body"))
			//.BorderBackgroundColor(this, &SGraphNode_Dialogue::GetBackgroundColor)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)			
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				SNew(SBox)
				.MinDesiredHeight(25.0f)
				[
					SNew(SVerticalBox)
					.Visibility(this, &SGraphNode_Dialogue::GetContentVisibility)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(4.0f, 0.0f, 4.0f, 5.0f))
					.HAlign(HAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							CreateTitleWidget(NodeTitle)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							NodeTitle.ToSharedRef()
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)		
						.MaxDesiredHeight(0.0f)
						.MaxDesiredHeight(DialogueSettings->NodeMaxBodySize)
						.Visibility(this, &SGraphNode_Dialogue::GetContentVisibility)
						[
							SNew(SScrollBox)			
							.ScrollBarStyle(&FCoreStyle::Get().GetWidgetStyle<FScrollBarStyle>("ScrollBar"))
							.ScrollBarThickness(FVector2D(4.0f, 4.0f))
							.ConsumeMouseWheel(EConsumeMouseWheel::Never)
							.ScrollBarPadding(1.0f)
							+ SScrollBox::Slot()
							[
								SNew(STextBlock)
								.Text(this, &SGraphNode_Dialogue::GetBodyText)
								.Justification(DialogueSettings->BodyJustifyLeft ? ETextJustify::Left : ETextJustify::Center)
								.AutoWrapText(true)
							]	
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 5.0f, 0.0f, 2.0f))
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("Graph.StateNode.Body"))
						.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.5f))
						.Padding(FMargin(0.0f, 1.0f, 0.0f, 0.0f))
						.Visibility(this, &SGraphNode_Dialogue::GetContextVisibility)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Justification(DialogueSettings->ContextJustifyLeft ? ETextJustify::Left : ETextJustify::Center)
						.Text(this, &SGraphNode_Dialogue::GetContextText)
						.Visibility(this, &SGraphNode_Dialogue::GetContextVisibility)
					]
				]
			]
		]

		// OUTPUT PIN AREA
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)
			.MinDesiredHeight(NodePadding.Bottom)
			[
				SAssignNew(RightNodeBox, SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.Padding(20.0f, 0.0f)
				.FillHeight(1.0f)
				[
					SAssignNew(OutputPinBox, SHorizontalBox)
				]
			]
		];

	//InlineEditableText->SetVisibility(EVisibility::Collapsed);


	// Error display
	InnerVerticalBox->AddSlot()
		.AutoHeight()
		.Padding(Settings->GetNonPinNodeBodyPadding())
		[
			ErrorReporting->AsWidget()
		];


	// Node body
	GetOrAddSlot( ENodeZone::Center )
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FDialogueStyle::Get()->GetBrush("DialogueEditor.Graph.DialogueNode.DebugBorder"))
			.BorderBackgroundColor(this, &SGraphNode_Dialogue::GetDebugBorderColor)
			.Padding(this, &SGraphNode_Dialogue::GetDebugBorderPadding)
			[
				SNew(SBox)
				.WidthOverride(200)
				[
					SAssignNew(MainVerticalBox, SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SOverlay)
						.AddMetaData<FGraphNodeMetaData>(TagMeta)
						+SOverlay::Slot()
						.Padding(Settings->GetNonPinNodeBodyPadding())
						[
							SNew(SImage)
							.Image(GetNodeBodyBrush())
							.ColorAndOpacity(this, &SGraphNode_Dialogue::GetNodeBodyColor)
						]
						+SOverlay::Slot()
						.Padding(Settings->GetNonPinNodeBodyPadding())
						[
							SNew(SImage)
							.Image(FDialogueStyle::Get()->GetBrush("DialogueEditor.Graph.DialogueNode.Border"))
							.ColorAndOpacity(this, &SGraphNode_Dialogue::GetBorderColor)
						]
						+SOverlay::Slot()
						[
							InnerVerticalBox.ToSharedRef()
						]
					]	
				]
			]
		];



	//~ SGraphNode Comment begin
	{
		bool SupportsBubble = true;
		if (GraphNode != nullptr)
		{
			SupportsBubble = GraphNode->SupportsCommentBubble();
		}

		if (SupportsBubble)
		{
			// Create comment bubble
			TSharedPtr<SCommentBubble> CommentBubble;
			const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

			SAssignNew(CommentBubble, SCommentBubble)
				.GraphNode(GraphNode)
				.Text(this, &SGraphNode::GetNodeComment)
				.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
				.OnToggled(this, &SGraphNode::OnCommentBubbleToggled)
				.ColorAndOpacity(CommentColor)
				.AllowPinning(true)
				.EnableTitleBarBubble(true)
				.EnableBubbleCtrls(true)
				.GraphLOD(this, &SGraphNode::GetCurrentLOD)
				.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

			GetOrAddSlot(ENodeZone::TopCenter)
				.SlotOffset(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetOffset))
				.SlotSize(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetSize))
				.AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
				.VAlign(VAlign_Top)
				[
					CommentBubble.ToSharedRef()
				];
		}
	}
	//~ SGraphNode Comment end

	CreatePinWidgets();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION



void SGraphNode_Dialogue::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility( TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced) );
	}

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			.Padding(20.0f,0.0f)
			[
				PinToAdd
			];
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		OutputPinBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillWidth(1.0f)
			[
				PinToAdd
			];
		OutputPins.Add(PinToAdd);
	}
}


TArray<FOverlayWidgetInfo> SGraphNode_Dialogue::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets;

	FVector2D Origin(0.0f, 0.0f);

	{
		FOverlayWidgetInfo Info;
		Info.OverlayOffset = FVector2D(Origin.X - (ExecutionWidget->GetDesiredSize().X * 0.5f), Origin.Y);
		Info.Widget = ExecutionWidget;

		Widgets.Add(Info);
	}

	float IconPadding = 0.0f;
	float NextX = 0;
	{
		FOverlayWidgetInfo Info;
		Info.Widget = IconWidget;
		Info.OverlayOffset = FVector2D(WidgetSize.X - (Info.Widget->GetDesiredSize().X * 0.5f + NextX), -(Info.Widget->GetDesiredSize().Y * 0.5f));

		NextX += Info.Widget->GetDesiredSize().X + IconPadding;

		Widgets.Add(Info);
	}	

	{
		FOverlayWidgetInfo Info;
		Info.Widget = ConditionWidget;
		Info.OverlayOffset = FVector2D(WidgetSize.X - (Info.Widget->GetDesiredSize().X * 0.5f + NextX), -(Info.Widget->GetDesiredSize().Y * 0.5f));

		NextX += Info.Widget->GetDesiredSize().X + IconPadding;

		Widgets.Add(Info);
	}

	{
		FOverlayWidgetInfo Info;
		Info.Widget = EventWidget;
		Info.OverlayOffset = FVector2D(WidgetSize.X - (Info.Widget->GetDesiredSize().X * 0.5f + NextX), -(Info.Widget->GetDesiredSize().Y * 0.5f));

		NextX += Info.Widget->GetDesiredSize().X + IconPadding;

		Widgets.Add(Info);
	}

	{
		FOverlayWidgetInfo Info;
		Info.Widget = SoundWidget;
		Info.OverlayOffset = FVector2D(WidgetSize.X - (Info.Widget->GetDesiredSize().X * 0.5f + NextX), -(Info.Widget->GetDesiredSize().Y * 0.5f));

		NextX += Info.Widget->GetDesiredSize().X + IconPadding;

		Widgets.Add(Info);
	}

	Algo::Reverse(Widgets);
	return Widgets;
}

void SGraphNode_Dialogue::UpdateCachedValues()
{
	UEdGraphNode_DialogueBase* DialogueBase = Cast<UEdGraphNode_DialogueBase>(GraphNode);
	if (DialogueBase)
	{
		CachedBodyText = DialogueBase->GetBodyText();
		CachedContextText = DialogueBase->GetContextText();

		CachedIconBrush = DialogueBase->GetNodeIcon();

		CachedBodyColor = DialogueBase->GetNodeBodyTintColor();
		CachedBorderColor = DialogueBase->GetNodeBorderTintColor();
	}
}

void SGraphNode_Dialogue::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty /*= true*/) 
{
	SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);
	
	UEdGraphNode_DialogueBase* DialogueNode = Cast<UEdGraphNode_DialogueBase>(GraphNode);
	if (DialogueNode)
	{
		UEdGraph_Dialogue* DialogueGraph = DialogueNode->GetDialogueEdGraph();
		if (DialogueGraph)
		{
			DialogueGraph->UpdateChildrenOrder(DialogueNode);
		}
	}
}


TSharedPtr<SGraphPin> SGraphNode_Dialogue::CreatePinWidget(UEdGraphPin* Pin) const
{
	return SNew(SGraphPin_Dialogue, Pin);
}

const FSlateBrush* SGraphNode_Dialogue::GetNodeBodyBrush() const
{
	UEdGraphNode_DialogueEntry* AsEntry = Cast<UEdGraphNode_DialogueEntry>(GraphNode);

	if (AsEntry && AsEntry->Name == NAME_None)
	{
		return FDialogueStyle::Get()->GetBrush("DialogueEditor.Graph.DialogueEntryDefault");
	}
	return FDialogueStyle::Get()->GetBrush("DialogueEditor.Graph.DialogueNode");
}

FSlateColor SGraphNode_Dialogue::GetDebugBorderColor() const
{
	FLinearColor Color = FLinearColor::Transparent;

	if (UEdGraphNode_DialogueBase* Node = CastChecked<UEdGraphNode_DialogueBase>(GraphNode))
	{		
		if (Node->bDebuggerMark_Active)
		{
			Color = DialogueSettings->DebuggerState_Active;
		}
		else if (Node->bDebuggerMark_Finished)
		{
			Color = DialogueSettings->DebuggerState_Completed;
		}
	}	

	return Color;
}

FMargin SGraphNode_Dialogue::GetDebugBorderPadding() const
{
	bool bDebugEnabled = false;
	if (UEdGraphNode_DialogueBase* Node = CastChecked<UEdGraphNode_DialogueBase>(GraphNode))
	{
		bDebugEnabled = Node->bDebuggerMark_Active || 
						Node->bDebuggerMark_Finished || 
						Node->bDebuggerMark_EntryAllowed ||
						Node->bDebuggerMark_EntryDenied;
	}

	return bDebugEnabled ? FMargin(4.0f, 4.0f, 4.0f, 4.0f) : FMargin(0.0f);
}

EVisibility SGraphNode_Dialogue::GetContentVisibility() const
{
	UEdGraphNode_DialogueNode* Node = Cast<UEdGraphNode_DialogueNode>(GraphNode);
	return (Node && Node->Node.IsEmpty()) ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SGraphNode_Dialogue::GetContextVisibility() const
{
	UEdGraphNode_DialogueBase* Node = CastChecked<UEdGraphNode_DialogueBase>(GraphNode);
	return ( Node && Node->GetHasContext() )? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SGraphNode_Dialogue::GetIndexVisibility() const
{
	if (GraphNode->IsA(UEdGraphNode_DialogueEntry::StaticClass()))
	{
		return EVisibility::Collapsed;
	}

	UEdGraphNode_DialogueBase* Node = CastChecked<UEdGraphNode_DialogueBase>(GraphNode);
	UEdGraphNode_DialogueBase* SelectedParentNode = nullptr;


	TSharedPtr<SGraphPanel> OwnerPanel = GetOwnerPanel();

	if (OwnerPanel && OwnerPanel->SelectionManager.GetSelectedNodes().Num() == 1)
	{
		if (OwnerPanel->SelectionManager.IsNodeSelected(Node))
		{
			return EVisibility::Visible;
		}

		UEdGraphPin* MyInputPin = Node->GetInputPin();
		for (UEdGraphPin* ParentPin : MyInputPin->LinkedTo)
		{
			UEdGraphNode_DialogueBase* ParentNode = CastChecked<UEdGraphNode_DialogueBase>(ParentPin->GetOwningNode());

			if (ParentNode && OwnerPanel->SelectionManager.IsNodeSelected(ParentNode))
			{
				SelectedParentNode = ParentNode;
				break;
			}
		}
	}

	return (SelectedParentNode != nullptr) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SGraphNode_Dialogue::GetIndexText() const
{
	int32 Index = 0;

	UEdGraphNode_DialogueBase* Node = CastChecked<UEdGraphNode_DialogueBase>(GraphNode);
	UEdGraphNode_DialogueBase* SelectedParentNode = nullptr;

	
	TSharedPtr<SGraphPanel> OwnerPanel = GetOwnerPanel();	

	if (OwnerPanel && OwnerPanel->SelectionManager.GetSelectedNodes().Num() == 1)
	{
		if (OwnerPanel->SelectionManager.IsNodeSelected(Node))
		{
			if (Node->GetInputPin() && Node->GetInputPin()->LinkedTo.Num() == 1)
			{
				UEdGraphNode_DialogueBase* ParentNode = CastChecked<UEdGraphNode_DialogueBase>(Node->GetInputPin()->LinkedTo[0]->GetOwningNode());
				if (ParentNode)
				{
					return FText::AsNumber(ParentNode->GetExecutionIndex(Node));
				}				
			}
			return LOCTEXT("IndexSelected", "");
		}

		UEdGraphPin* MyInputPin = Node->GetInputPin();
		for (UEdGraphPin* ParentPin : MyInputPin->LinkedTo)
		{
			UEdGraphNode_DialogueBase* ParentNode = CastChecked<UEdGraphNode_DialogueBase>(ParentPin->GetOwningNode());
			
			if (ParentNode && OwnerPanel->SelectionManager.IsNodeSelected(ParentNode))
			{			
				SelectedParentNode = ParentNode;
				break;
			}
		}	
	}

	if (SelectedParentNode)
	{
		Index = SelectedParentNode->GetExecutionIndex(Node);
	}

	return FText::AsNumber(Index);
}

FSlateColor SGraphNode_Dialogue::GetIndexColor() const
{
	UEdGraphNode_DialogueBase* Node = CastChecked<UEdGraphNode_DialogueBase>(GraphNode);

	TSharedPtr<SGraphPanel> OwnerPanel = GetOwnerPanel();

	if (OwnerPanel && OwnerPanel->SelectionManager.IsNodeSelected(Node))
	{
		return FDialogueStyle::Get()->GetSlateColor("DialogueEditor.Graph.DialogueNode.Index.Selected");
	}
	return FDialogueStyle::Get()->GetSlateColor("DialogueEditor.Graph.DialogueNode.Index.Default");
}

EVisibility SGraphNode_Dialogue::GetIconVisibility() const
{
	return EVisibility::Visible;
}

EVisibility SGraphNode_Dialogue::GetConditionVisibility() const
{
	UEdGraphNode_DialogueNode* Node = Cast<UEdGraphNode_DialogueNode>(GraphNode);
	return (Node && Node->Node.HasCondition()) ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility SGraphNode_Dialogue::GetEventVisibility() const
{
	UEdGraphNode_DialogueNode* Node = Cast<UEdGraphNode_DialogueNode>(GraphNode);
	return (Node && Node->Node.HasEvents()) ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility SGraphNode_Dialogue::GetSoundVisibility() const
{
	UEdGraphNode_DialogueNode* Node = Cast<UEdGraphNode_DialogueNode>(GraphNode);
	return (Node && Node->Node.Sound != nullptr) ? EVisibility::Visible : EVisibility::Hidden;
}

#undef LOCTEXT_NAMESPACE
