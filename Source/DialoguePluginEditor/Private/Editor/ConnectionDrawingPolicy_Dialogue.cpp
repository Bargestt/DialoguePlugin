

#include "ConnectionDrawingPolicy_Dialogue.h"
#include "EdGraphNode_DialogueBase.h"
#include <SNodePanel.h>
#include <SGraphNode.h>
#include <SGraphPanel.h>
#include "DialogueEditorSettings.h"

FConnectionDrawingPolicy_Dialogue::FConnectionDrawingPolicy_Dialogue(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
{
}

void FConnectionDrawingPolicy_Dialogue::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params)
{	
	const UDialogueEditorSettings* DialogueSettings = UDialogueEditorSettings::Get();

	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;
	Params.WireThickness = DialogueSettings->WireThickness;
	Params.WireColor = DialogueSettings->WireColor;

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
	{		
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
	}

	UEdGraphNode_DialogueBase* FromNode = OutputPin ? Cast<UEdGraphNode_DialogueBase>(OutputPin->GetOwningNode()) : NULL;
	UEdGraphNode_DialogueBase* ToNode = InputPin ? Cast<UEdGraphNode_DialogueBase>(InputPin->GetOwningNode()) : NULL;

	if (ToNode && FromNode)
	{
		if (FromNode->bDebuggerMark_Active)
		{
			if (ToNode->bDebuggerMark_EntryDenied)
			{
				Params.WireThickness *= 2.0f;
				Params.WireColor = DialogueSettings->DebuggerState_EntryDenied;
				Params.bUserFlag1 = true;
			}
			else if (ToNode->bDebuggerMark_EntryAllowed)
			{
				Params.WireThickness *= 2.0f;
				Params.WireColor = DialogueSettings->DebuggerState_EntryAllowed;
				Params.bUserFlag1 = true;
			}
		}
	}


	bool bSelected = false;
	if (!bSelected && OutputPin)
	{	
		class UEdGraphNode* OutputNode = OutputPin->GetOwningNode();
		if (const TSharedPtr<SGraphNode>* Widget = NodeToWidgetMap.Find(OutputNode))
		{
			bSelected = (*Widget)->GetOwnerPanel()->SelectionManager.IsNodeSelected(OutputNode);
		}
	}
	if (!bSelected && InputPin)
	{
		class UEdGraphNode* InputNode = InputPin->GetOwningNode();
		if (const TSharedPtr<SGraphNode>* Widget = NodeToWidgetMap.Find(InputNode))
		{
			bSelected = (*Widget)->GetOwnerPanel()->SelectionManager.IsNodeSelected(InputNode);
		}
	}

	Params.bUserFlag2 = bSelected;
	if (bSelected)
	{		
		Params.WireThickness *= DialogueSettings->WireThicknessScale_Selected;
		Params.WireColor *= DialogueSettings->WireColor_Selected;
	}
}



void FConnectionDrawingPolicy_Dialogue::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	// Build an acceleration structure to quickly find geometry for the nodes
	NodeWidgetMap.Empty();
	for (int32 NodeIndex = 0; NodeIndex < ArrangedNodes.Num(); ++NodeIndex)
	{
		FArrangedWidget& CurWidget = ArrangedNodes[NodeIndex];
		TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
		NodeWidgetMap.Add(ChildNode->GetNodeObj(), NodeIndex);
		NodeToWidgetMap.Add(ChildNode->GetNodeObj(), ChildNode);
	}

	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

void FConnectionDrawingPolicy_Dialogue::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params)
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);

	const bool bGoingDown = StartCenter.Y < EndCenter.Y;
	if (bGoingDown)
	{
		const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

		// Find the (approximate) closest points between the two boxes
		const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
		const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

		DrawSplineWithArrow(StartAnchorPoint, EndAnchorPoint, Params);
	}
	else
	{
		const FVector2D StartAnchorPoint = FGeometryHelper::VerticalMiddleRightOf(StartGeom);
		const FVector2D EndAnchorPoint = FGeometryHelper::VerticalMiddleLeftOf(EndGeom);

		DrawSplineWithArrow(StartAnchorPoint, EndAnchorPoint, Params);
	}
}


void FConnectionDrawingPolicy_Dialogue::DrawSplineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params)
{
	const bool bGoingDown = StartAnchorPoint.Y < EndAnchorPoint.Y;
	if (!bGoingDown)
	{
		FConnectionParams Params2 = Params;
		Params2.WireThickness *= UDialogueEditorSettings::Get()->ReturnWireThicknessScale;

		if (!Params.bUserFlag1)
		{
			if (Params2.bUserFlag2)
			{
				Params2.WireColor = UDialogueEditorSettings::Get()->ReturnWireColor_Selected;
			}
			else
			{
				Params2.WireColor = UDialogueEditorSettings::Get()->ReturnWireColor;
			}
		}		

		FConnectionDrawingPolicy::DrawSplineWithArrow(StartAnchorPoint, EndAnchorPoint, Params2);
		return;
	}

	const float LineSeparationAmount = 4.5f;

	const FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
	const FVector2D UnitDelta = DeltaPos.GetSafeNormal();
	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	// Come up with the final start/end points
	const FVector2D DirectionBias = Normal * LineSeparationAmount;
	const FVector2D LengthBias = ArrowRadius.X * UnitDelta;
	const FVector2D StartPoint = StartAnchorPoint + DirectionBias + LengthBias;
	const FVector2D EndPoint = EndAnchorPoint + DirectionBias - LengthBias;

	DrawConnection(WireLayerID, StartPoint, EndPoint, Params);

	// Draw the arrow
	if (ArrowImage != nullptr)
	{
		const FVector2D ArrowDrawPos = EndPoint - ArrowRadius;
		const float AngleInRadians = FMath::Atan2(DeltaPos.Y, DeltaPos.X);


		FSlateDrawElement::MakeRotatedBox(
			DrawElementsList,
			ArrowLayerID,
			FPaintGeometry(ArrowDrawPos, ArrowImage->ImageSize * ZoomFactor, ZoomFactor),
			ArrowImage,
			ESlateDrawEffect::None,
			AngleInRadians,
			TOptional<FVector2D>(),
			FSlateDrawElement::RelativeToElement,
			Params.WireColor
		);
	}
}


void FConnectionDrawingPolicy_Dialogue::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	FVector2D StartAnchorPoint = StartPoint;
	FVector2D EndAnchorPoint = EndPoint;

	const bool bGoingDown = StartPoint.Y < EndPoint.Y;
	if (bGoingDown)
	{		
		if (Pin->Direction == EGPD_Output)
		{
			const FVector2D StartCenter = FGeometryHelper::CenterOf(PinGeometry);
			const FVector2D SeedPoint = (StartCenter + EndPoint) * 0.5f;

			StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(PinGeometry, SeedPoint);
		}
		else if (Pin->Direction == EGPD_Input)
		{
			const FVector2D EndCenter = FGeometryHelper::CenterOf(PinGeometry);
			const FVector2D SeedPoint = (EndCenter + StartPoint) * 0.5f;

			EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(PinGeometry, SeedPoint);
		}
	}

	FConnectionDrawingPolicy::DrawPreviewConnector(PinGeometry, StartAnchorPoint, EndAnchorPoint, Pin);
}

FVector2D FConnectionDrawingPolicy_Dialogue::ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const
{
	const bool bGoingDown = Start.Y < End.Y;
	if (bGoingDown)
	{
		return FVector2D::ZeroVector;
	}
	else
	{
		return FConnectionDrawingPolicy::ComputeSplineTangent(Start, End);
	}
}

void FConnectionDrawingPolicy_Dialogue::DetermineLinkGeometry(FArrangedChildren& ArrangedNodes, TSharedRef<SWidget>& OutputPinWidget, UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*out*/ FArrangedWidget*& StartWidgetGeometry, /*out*/ FArrangedWidget*& EndWidgetGeometry)
{
	FConnectionDrawingPolicy::DetermineLinkGeometry(ArrangedNodes, OutputPinWidget, OutputPin, InputPin, StartWidgetGeometry, EndWidgetGeometry);

	if (OutputPin && InputPin && StartWidgetGeometry && EndWidgetGeometry)
	{
		bool bGoingDown = FGeometryHelper::CenterOf(StartWidgetGeometry->Geometry).Y < FGeometryHelper::CenterOf(EndWidgetGeometry->Geometry).Y;

		if (!bGoingDown)
		{
			UEdGraphNode* OutputNode = OutputPin->GetOwningNode();
			UEdGraphNode* InputNode = InputPin->GetOwningNode();

			if (const int32* NodeIndex = NodeWidgetMap.Find(InputNode))
			{
				EndWidgetGeometry = &ArrangedNodes[*NodeIndex];
			}

			if (const int32* NodeIndex = NodeWidgetMap.Find(OutputNode))
			{
				StartWidgetGeometry = &ArrangedNodes[*NodeIndex];
			}
		}
	}	
}


