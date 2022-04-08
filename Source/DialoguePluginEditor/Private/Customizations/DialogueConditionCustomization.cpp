
#include "DialogueConditionCustomization.h"

#include "PropertyEditing.h"
#include <PropertyCustomizationHelpers.h>
#include <Widgets/Text/SRichTextBlock.h>
#include <IPropertyUtilities.h>



#define LOCTEXT_NAMESPACE "DialogueConditionCustomization"

class SConditionTitile : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SConditionTitile) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<IPropertyHandle> InPropertyHandle)
	{
		ConditionHandle = InPropertyHandle;
		ConditionHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &SConditionTitile::UpdateTitle));
		IndentSize = 4;	

		TimeLeft = 0.0f;
		ResetTime = 3.0f;	
		
		
		UpdateTitle();

		TitleWidget = 
			SNew(SRichTextBlock)
			.DecoratorStyleSet(&FEditorStyle::Get())
			.TextStyle(FEditorStyle::Get(), "NormalText")
			.Text(this, &SConditionTitile::GetTitle);				

		TooltipWidget = 
			SNew(SToolTip)
			[
				SNew(SBox)
				.MinDesiredWidth(300.0f)
				[
					SNew(SRichTextBlock)
					.DecoratorStyleSet(&FEditorStyle::Get())
					.TextStyle(FEditorStyle::Get(), "NormalText")
					.Text(this, &SConditionTitile::GetTextTooltip)
				]
			];



		EditorWidget = 
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().Padding(5.0f, 0.0f)
			[
				SNew(SClassPropertyEntryBox)
				.MetaClass(UObject::StaticClass())
				.RequiredInterface(UDialogueConditionInterface::StaticClass())
				.AllowAbstract(false)
				.AllowNone(true)
				.ShowDisplayNames(true)
				.OnSetClass(this, &SConditionTitile::OnSetClass)
				.SelectedClass(this, &SConditionTitile::GetCurrentClass)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).HAlign(HAlign_Center)
			[
				PropertyCustomizationHelpers::MakeClearButton(FSimpleDelegate::CreateSP(this, &SConditionTitile::Clear))
			];


			
		SetEditorVisibility(false);

		ChildSlot
		[
			SNew(SOverlay)					
			+ SOverlay::Slot()
			[
				SNew(SButton)
				.ButtonStyle(&FEditorStyle::Get().GetWidgetStyle< FButtonStyle >("ToggleButton"))
				.OnClicked(this, &SConditionTitile::ToggleButtonClicked)
				.Visibility(this, &SConditionTitile::GetButtonVisibility)
				.ToolTip(TooltipWidget)
			]
			+ SOverlay::Slot()
			[
				EditorWidget.ToSharedRef()
			]
			+ SOverlay::Slot().VAlign(VAlign_Center).Padding(5.0f, 0.0f)
			[
				TitleWidget.ToSharedRef()
			]	
		];
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

		if (TimeLeft > 0)
		{
			if (IsHovered())
			{
				TimeLeft = ResetTime;
			}
			else
			{
				TimeLeft -= InDeltaTime;
				if (TimeLeft < 0)
				{
					SetEditorVisibility(false);
					TimeLeft = -1;
				}
			}
		}		
	}

private:
	FReply ToggleButtonClicked()
	{
		if (TimeLeft > 0)
		{
			SetEditorVisibility(false);
			TimeLeft = 0;
		}
		else
		{
			SetEditorVisibility(true);
			TimeLeft = ResetTime;
		}

		return FReply::Handled();
	}

	EVisibility GetButtonVisibility() const
	{
		return (ConditionHandle->GetNumPerObjectValues() == 1) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	FSlateColor GetButtonColor() const
	{
		return (TimeLeft > 0) ? FCoreStyle::Get().GetSlateColor("SelectionColor_Pressed") : FSlateColor(FLinearColor::White);
	}

	FText GetEditorText() const
	{	
		return FText::FromString(TEXT("Editor: ") + FString::SanitizeFloat(TimeLeft));
	}

	void SetEditorVisibility(bool bVisible)
	{
		EditorWidget->SetVisibility(bVisible ? EVisibility::Visible : EVisibility::Collapsed);
		TitleWidget->SetVisibility(!bVisible ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	}


	void UpdateTitle()
	{
		UObject* ObjectValue = nullptr;
		FPropertyAccess::Result Result = ConditionHandle->GetValue(ObjectValue);

		if (Result == FPropertyAccess::Fail)
		{
			Title = LOCTEXT("TitleFail", "Fail");
			Tooltip = Title;
			return;
		}
		else if (Result == FPropertyAccess::MultipleValues)
		{
			Title = LOCTEXT("TitleMultiple", "Multiple Values");
			Tooltip = Title;
			return;
		}

		if (ObjectValue == nullptr)
		{
			Title = LOCTEXT("TitleNone", "None");
			Tooltip = Title;
			return;
		}

		Title = FText::FromString(IDialogueConditionInterface::GetTitleSafe(ObjectValue, false));

		FString IndentedString;
		int32 Error = ParseIndent(IDialogueConditionInterface::GetTitleSafe(ObjectValue, true), IndentedString, IndentSize);
		if (Error > 0)
		{
			IndentedString += FString::Printf(TEXT("\nmissing %d brackets"), Error);
		}
		else if (Error < 0)
		{
			IndentedString += FString::Printf(TEXT("\nextra %d brackets"), Error);
		}

		Tooltip = FText::FromString(IndentedString);
	}

	static int32 ParseIndent(FString InString, FString& OutString, int32 IndentSize)
	{
		int32 Depth = 0;
		bool bDepthChanged = false;
		for (int32 Index = 0; Index < InString.Len(); Index++)
		{
			if (InString[Index] == '(')
			{
				Depth++;
				bDepthChanged = true;
			}
			else if (InString[Index] == ')')
			{
				Depth--;
				bDepthChanged = true;
			}
			else
			{
				if (bDepthChanged)
				{
					OutString += TEXT("\n") + FString::ChrN(FMath::Max(0, Depth * IndentSize), ' ');
				}
				OutString += InString[Index];
				bDepthChanged = false;
			}
		}
		OutString.RemoveFromStart(TEXT("\n"));
		return Depth;
	}


	void Clear()
	{
		OnSetClass(nullptr);
	}

	const UClass* GetCurrentClass() const
	{
		UObject* ObjectValue = nullptr;
		FPropertyAccess::Result Result = ConditionHandle->GetValue(ObjectValue);

		return (Result == FPropertyAccess::Success && ObjectValue) ? ObjectValue->GetClass() : nullptr;
	}

	void OnSetClass(const UClass* NewClass)
	{
		UObject* ObjectValue = nullptr;
		FPropertyAccess::Result Result = ConditionHandle->GetValue(ObjectValue);

		if (Result != FPropertyAccess::Success)
		{
			return;
		}	
		
		if (ObjectValue && ObjectValue->GetClass() == NewClass)
		{
			return;
		}

		if (NewClass == nullptr)
		{
			ConditionHandle->SetValue(NAME_None);
		}
		else
		{
			TArray<UObject*> Outers;
			ConditionHandle->GetOuterObjects(Outers);
			if (Outers.Num() == 0)
			{
				return;
			}
			bool bIsStatic = IDialogueConditionInterface::Execute_IsConditionStatic(NewClass->GetDefaultObject());
			
			UObject* NewValue = bIsStatic ? NewClass->GetDefaultObject() : NewObject<UObject>(Outers[0], NewClass, NAME_None, RF_Transactional);
			ConditionHandle->SetValueFromFormattedString(NewValue->GetPathName());
		}
	}

private:
	TSharedPtr<IPropertyHandle> ConditionHandle;

	TSharedPtr<SWidget> TitleWidget;
	TSharedPtr<SToolTip> TooltipWidget;
	TSharedPtr<SWidget> EditorWidget;

	FText Title;
	FText GetTitle() const { return Title; }

	FText Tooltip;
	FText GetTextTooltip() const { return Tooltip; }

	float TimeLeft;
	float ResetTime;
	int32 IndentSize;
};


void FDialogueConditionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{	
	HeaderRow
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(200)
	.MaxDesiredWidth(4096)
	[
		SNew(SConditionTitile, PropertyHandle->GetChildHandle(0))
	];
}


void FDialogueConditionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyHandle> ObjectProperty = PropertyHandle->GetChildHandle(0);
	
	UObject* ObjectValue = nullptr;
	FPropertyAccess::Result Result = ObjectProperty->GetValue(ObjectValue);
	if (Result == FPropertyAccess::Success && ObjectValue && !ObjectValue->HasAnyFlags(RF_ClassDefaultObject))
	{
		uint32 NumChildren = INDEX_NONE;
		ObjectProperty->GetNumChildren(NumChildren);
		if (NumChildren > 0)
		{
			TSharedPtr<IPropertyHandle> ChildHandle = ObjectProperty->GetChildHandle(0);

			uint32 ChildProps = INDEX_NONE;
			ChildHandle->GetNumChildren(ChildProps);
			for (uint32 Index = 0; Index < ChildProps; Index++)
			{
				TSharedPtr<IPropertyHandle> PropHandle = ChildHandle->GetChildHandle(Index);
				if(PropHandle->GetProperty()->HasAnyPropertyFlags(CPF_Edit) && 
					!PropHandle->GetProperty()->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
				{ 
					ChildBuilder.AddProperty(PropHandle.ToSharedRef());
				}				
			}
		}
	}
}

void FDialogueConditionArrayCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
	

	TSharedPtr<IPropertyHandle> ArrayPropertyHandle = PropertyHandle->GetChildHandle(0);	
	TSharedPtr<class IPropertyHandleArray> ArrayHandle = ArrayPropertyHandle->AsArray();

	FSimpleDelegate RefreshDelegate = FSimpleDelegate::CreateSP(this, &FDialogueConditionArrayCustomization::Refresh);

	ArrayPropertyHandle->SetOnPropertyValueChanged(RefreshDelegate);	
	ArrayHandle->SetOnNumElementsChanged(RefreshDelegate);
	
	ChildBuilder.AddProperty(ArrayPropertyHandle.ToSharedRef())
		.CustomWidget(false)
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(200)
		.MaxDesiredWidth(4096)
		[
			ArrayPropertyHandle->CreatePropertyValueWidget(false)
		];


	uint32 ChildrenNum;
	ArrayHandle->GetNumElements(ChildrenNum);
	for (uint32 Index = 0; Index < ChildrenNum; Index++)
	{
		TSharedRef<IPropertyHandle> Element = ArrayHandle->GetElement(Index);
		ChildBuilder.AddProperty(Element);
	}
}

void FDialogueConditionArrayCustomization::Refresh()
{
	if (PropertyUtilities)
	{
		PropertyUtilities->ForceRefresh();
	}
}

#undef LOCTEXT_NAMESPACE


