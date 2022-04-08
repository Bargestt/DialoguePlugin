// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



#include "AssetEditor_Dialogue.h"

#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "GraphEditor.h"
#include "SEditorViewport.h"
#include "Widgets/Docking/SDockTab.h"
#include "Editor/EditorEngine.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Application/SlateApplication.h"

#include <SSingleObjectDetailsPanel.h>
#include "EditorCommands_Dialogue.h"

#include "Dialogue.h"

#include <Kismet2/BlueprintEditorUtils.h>

#include "EdGraph_Dialogue.h"
#include "EdGraphSchema_Dialogue.h"
#include <EdGraphUtilities.h>
#include "HAL/PlatformApplicationMisc.h"
#include "EdGraphNode_DialogueBase.h"
#include "EdGraphNode_DialogueNode.h"
#include "DialogueEditorSettings.h"
#include "DialogueDebugger.h"
#include "DialogueExecutor.h"

#define LOCTEXT_NAMESPACE "AssetEditor_Dialogue"

const FName DialogueEditorAppName = FName(TEXT("DialogueEditorApp"));

struct FEditorTabs_Dialogue
{
	// Tab identifiers
	static const FName DetailsID;
	static const FName ViewportID;
	static const FName SnippetsID;
};

const FName FEditorTabs_Dialogue::DetailsID(TEXT("Details"));
const FName FEditorTabs_Dialogue::ViewportID(TEXT("Viewport"));
const FName FEditorTabs_Dialogue::SnippetsID(TEXT("Snippets"));




class SSnippetsSelector : public SCompoundWidget
{
public:
	using SnippetEntryPtr = TSharedPtr<FDialogueNodeSnippet>;


	DECLARE_DELEGATE_OneParam(FSnippetSelected, FString /*SnippetContent*/);

	SLATE_BEGIN_ARGS(SSnippetsSelector) {}
	SLATE_EVENT(FSnippetSelected, OnSelected)
	SLATE_EVENT(FSnippetSelected, OnApplySelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FAssetEditor_Dialogue* OwningEditor)
	{
		OnSelected = InArgs._OnSelected;
		OnApplySelected = InArgs._OnApplySelected;

		RefreshItems();

		FToolBarBuilder ToolbarBuilder(OwningEditor->GetToolkitCommands(), FMultiBoxCustomization::None, nullptr, true);

		// Use a custom style		
		ToolbarBuilder.SetIsFocusable(false);		

		ToolbarBuilder.BeginSection("Main");
		{
			ToolbarBuilder.AddToolBarButton(FDialogueEditorCommands::Get().AddSnippet);			
		}
		ToolbarBuilder.EndSection();

		ToolbarBuilder.BeginSection("Utility");
		{
			ToolbarBuilder.AddToolBarButton(FDialogueEditorCommands::Get().RefreshSnippets);
		}
		ToolbarBuilder.EndSection();

		ChildSlot
		[		
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(15.0f, 5.0f, 5.0f, 10.0f))
			[
				ToolbarBuilder.MakeWidget()
			]
			+ SVerticalBox::Slot().Padding(FMargin(15.0f, 5.0f, 5.0f, 10.0f))
			[
				SAssignNew(List, SListView<SnippetEntryPtr>)
				.ItemHeight(24)
				.ListItemsSource(&Items)
				.SelectionMode(ESelectionMode::Single)
				.OnSelectionChanged(this, &SSnippetsSelector::OnSelectionChanged)
				.OnMouseButtonDoubleClick(this, &SSnippetsSelector::OnDoubleClick)
				.OnGenerateRow(this, &SSnippetsSelector::GenerateRow)
			]
		];
	}


	void RefreshItems()
	{
		TArray<FDialogueNodeSnippet> Snippets;

		const UDialogueEditorSettings* Settings = UDialogueEditorSettings::Get();
		Snippets = Settings->Snippets;

		for (FSoftObjectPath TablePath : Settings->SnippetTables)
		{
			UDataTable* Table = Cast<UDataTable>(TablePath.TryLoad());

			if (Table && Table->GetRowStruct() == FDialogueNodeSnippet::StaticStruct())
			{
				TArray<FDialogueNodeSnippet*> Rows;
				Table->GetAllRows<FDialogueNodeSnippet>(TEXT("DialogueEditor"), Rows);

				for (FDialogueNodeSnippet* Row : Rows)
				{
					if (Row)
					{					
						Snippets.Add(*Row);
					}
				}
			}
		}

		Snippets.Sort([](const FDialogueNodeSnippet& A, const FDialogueNodeSnippet& B) 
			{ 
				return A.DisplayCategory.Equals(B.DisplayCategory) ? (A.DisplayName <= B.DisplayName) : (A.DisplayCategory <= B.DisplayCategory);
			});

		Items.Empty(Snippets.Num());

		for (FDialogueNodeSnippet& Snippet : Snippets)
		{
			Items.Add(MakeShared<FDialogueNodeSnippet>(Snippet));
		}
	}

	TSharedRef<ITableRow> GenerateRow(SnippetEntryPtr Data, const TSharedRef<STableViewBase>& OwnerTable)
	{		
		const int32 ChopLimit = 30;
		
		FString DisplayStr = Data->Value;

		int32 EndChar = DisplayStr.Find(TEXT("\n"));
		if (EndChar > 0)
		{
			DisplayStr = DisplayStr.Left(EndChar);
		}
		
		DisplayStr = (DisplayStr.Len() > ChopLimit + 3 ) ? (DisplayStr.Left(ChopLimit) + TEXT("...")) : DisplayStr;

		return SNew(STableRow<SnippetEntryPtr>, OwnerTable).Padding(FMargin(0.0f, 2.0f, 0.0f, 2.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f, 0.0f)
			[
				SNew(STextBlock).Text(FText::FromString(Data->DisplayName))
			]
			+ SHorizontalBox::Slot().HAlign(HAlign_Right)
			[
				SNew(STextBlock).Text(FText::FromString(DisplayStr)).Justification(ETextJustify::Right)
			]
		];
	}

	void OnSelectionChanged(SnippetEntryPtr Data, ESelectInfo::Type Type)
	{				
		OnSelected.ExecuteIfBound(Data.IsValid() ? Data->Value : TEXT(""));
	}

	void OnDoubleClick(SnippetEntryPtr Data)
	{
		OnApplySelected.ExecuteIfBound(Data.IsValid() ? Data->Value : TEXT(""));		
	}

private:
	TSharedPtr<SListView<SnippetEntryPtr>> List;

	TArray<SnippetEntryPtr> Items;

	FSnippetSelected OnSelected;

	FSnippetSelected OnApplySelected;
};



FAssetEditor_Dialogue::FAssetEditor_Dialogue()
{
	AssetBeingEdited = nullptr;
}

FAssetEditor_Dialogue::~FAssetEditor_Dialogue()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}

	Debugger.Reset();
}

TSharedRef<SDockTab> FAssetEditor_Dialogue::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_Dialogue", "Dialogue");

	CreateGraphCommandList();

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FAssetEditor_Dialogue::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FAssetEditor_Dialogue::OnNodeDoubleClicked);

	ViewportWidget = SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(AssetBeingEdited->EdGraph)
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);


	return SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"))
		[
			ViewportWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FAssetEditor_Dialogue::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	if (!DetailsWidget.IsValid())
	{
		FDetailsViewArgs DetailArgs;
		DetailArgs.bHideSelectionTip = true;
		//Args.NotifyHook = this;

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		DetailsWidget = PropertyModule.CreateDetailView(DetailArgs);
		DetailsWidget->SetObject(AssetBeingEdited);
		DetailsWidget->OnFinishedChangingProperties().AddSP(this, &FAssetEditor_Dialogue::OnFinishedChangingProperties);
	}

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTab_Title", "Details"))
		[
			DetailsWidget.ToSharedRef()
		];
}



TSharedRef<SDockTab> FAssetEditor_Dialogue::SpawnTab_Snippets(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("SnippetsTab_Title", "Snippets"))
		[
			SNew(SSnippetsSelector, this)
			.OnSelected(this, &FAssetEditor_Dialogue::SelectSnippet)
			.OnApplySelected(this, &FAssetEditor_Dialogue::ApplySnippet)
		];
}

void FAssetEditor_Dialogue::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		FSlateApplication::Get().DismissAllMenus();
	}
}

void FAssetEditor_Dialogue::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{			
		FSlateApplication::Get().DismissAllMenus();
	}
}


void FAssetEditor_Dialogue::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_DialogueEditor", "Dialogue Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FEditorTabs_Dialogue::ViewportID, FOnSpawnTab::CreateSP(this, &FAssetEditor_Dialogue::SpawnTab_Viewport))
		.SetDisplayName( LOCTEXT("ViewportTab", "Viewport") )
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));

	InTabManager->RegisterTabSpawner(FEditorTabs_Dialogue::DetailsID, FOnSpawnTab::CreateSP(this, &FAssetEditor_Dialogue::SpawnTab_Details))
		.SetDisplayName( LOCTEXT("DetailsTabLabel", "Details") )
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FEditorTabs_Dialogue::SnippetsID, FOnSpawnTab::CreateSP(this, &FAssetEditor_Dialogue::SpawnTab_Snippets))
		.SetDisplayName(LOCTEXT("SnippetsTabLabel", "Snippets"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FAssetEditor_Dialogue::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FEditorTabs_Dialogue::ViewportID);
	InTabManager->UnregisterTabSpawner(FEditorTabs_Dialogue::DetailsID);	
	InTabManager->UnregisterTabSpawner(FEditorTabs_Dialogue::SnippetsID);
}


void FAssetEditor_Dialogue::InitDialogueEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UDialogue* InitDialogue)
{
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseOtherEditors(InitDialogue, this);
	AssetBeingEdited = InitDialogue;

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}

	RestoreDialogueGraph();

	Debugger = MakeShareable(new FDialogueDebugger);
	Debugger->Setup(AssetBeingEdited, SharedThis(this));
	BindDebuggerCommands();

	BindCommands();

	// Default layout	
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_DialogueEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)			
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.75f)
					->SetHideTabWell(true)
					->AddTab(FEditorTabs_Dialogue::ViewportID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.25f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.75f)
						->SetHideTabWell(false)
						->AddTab(FEditorTabs_Dialogue::DetailsID, ETabState::OpenedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.25f)
						->SetHideTabWell(false)
						->AddTab(FEditorTabs_Dialogue::SnippetsID, ETabState::OpenedTab)
					)					
				)
			)
		);

	// Initialize the asset editor
	InitAssetEditor(Mode, InitToolkitHost, DialogueEditorAppName, StandaloneDefaultLayout, /*bCreateDefaultStandaloneMenu=*/ true, /*bCreateDefaultToolbar=*/ true, InitDialogue);
	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

UEdGraph_Dialogue* FAssetEditor_Dialogue::GetGraph() const
{
	UEdGraph_Dialogue* EdGraph = Cast<UEdGraph_Dialogue>(AssetBeingEdited->EdGraph);
	check(EdGraph != nullptr);

	return EdGraph;
}

//////////////////////////////////////////////////////////////////////////
//	FAssetEditorToolkit interface

FName FAssetEditor_Dialogue::GetToolkitFName() const
{
	return FName("DialogueEditor");
}

FText FAssetEditor_Dialogue::GetBaseToolkitName() const
{
	return LOCTEXT("DialogueEditorAppLabel", "Dialogue Editor");
}

FText FAssetEditor_Dialogue::GetToolkitName() const
{
	return FText::FromString(AssetBeingEdited->GetName());
}

FText FAssetEditor_Dialogue::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(AssetBeingEdited);
}

FLinearColor FAssetEditor_Dialogue::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FAssetEditor_Dialogue::GetWorldCentricTabPrefix() const
{
	return TEXT("DialogueEditor");
}

void FAssetEditor_Dialogue::SaveAsset_Execute()
{
	if (AssetBeingEdited != nullptr)
	{
		RebuildDialogueGraph();
	}

	FAssetEditorToolkit::SaveAsset_Execute();
}





void FAssetEditor_Dialogue::CreateGraphCommandList()
{
	if (GraphEditorCommands.IsValid())
	{
		return;
	}
	GraphEditorCommands = MakeShareable(new FUICommandList);	

	FGenericCommands::Register();	
	const FGenericCommands& Commands = FGenericCommands::Get();

	GraphEditorCommands->MapAction(
		Commands.SelectAll,
		FExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::CanSelectAllNodes)
	);

	GraphEditorCommands->MapAction(
		Commands.Delete,
		FExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::CanDeleteNodes)
	);

	GraphEditorCommands->MapAction(
		Commands.Copy,
		FExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(
		Commands.Cut,
		FExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::CanCutNodes)
	);

	GraphEditorCommands->MapAction(
		Commands.Paste,
		FExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(
		Commands.Duplicate,
		FExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::CanDuplicateNodes)
	);
}

void FAssetEditor_Dialogue::BindCommands()
{
	FDialogueEditorCommands::Register();
	const FDialogueEditorCommands& Commands = FDialogueEditorCommands::Get();

	ToolkitCommands->MapAction(
		Commands.AddSnippet,
		FExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::ApplySelectedSnippet),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_Dialogue::CanApplySnippet)
	);
}


void FAssetEditor_Dialogue::BindDebuggerCommands()
{
	FDialogueDebuggerCommands::Register();
	const FDialogueDebuggerCommands& Commands = FDialogueDebuggerCommands::Get();

	TSharedRef<FDialogueDebugger> DebuggerOb = Debugger.ToSharedRef();

	ToolkitCommands->MapAction(
		Commands.StepBack,
		FExecuteAction::CreateSP(DebuggerOb, &FDialogueDebugger::StepBack),
		FCanExecuteAction::CreateSP(DebuggerOb, &FDialogueDebugger::CanStepBack));

	ToolkitCommands->MapAction(
		Commands.StepForward,
		FExecuteAction::CreateSP(DebuggerOb, &FDialogueDebugger::StepForward),
		FCanExecuteAction::CreateSP(DebuggerOb, &FDialogueDebugger::CanStepForward));

	ToolkitCommands->MapAction(
		Commands.StepToBegin,
		FExecuteAction::CreateSP(DebuggerOb, &FDialogueDebugger::StepToBegin),
		FCanExecuteAction::CreateSP(DebuggerOb, &FDialogueDebugger::CanStepToBegin));

	ToolkitCommands->MapAction(
		Commands.StepToEnd,
		FExecuteAction::CreateSP(DebuggerOb, &FDialogueDebugger::StepToEnd),
		FCanExecuteAction::CreateSP(DebuggerOb, &FDialogueDebugger::CanStepToEnd));
}

void FAssetEditor_Dialogue::ExtendToolbar()
{
	struct Local
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder, TWeakPtr<FAssetEditor_Dialogue> Editor)
		{
			TSharedPtr<FAssetEditor_Dialogue> EditorPtr = Editor.Pin();


			ToolbarBuilder.BeginSection("Debugger");
			{			
				const bool bCanShowDebugger = EditorPtr->IsDebuggerReady();
				if (bCanShowDebugger)
				{
					TSharedRef<SWidget> SelectionBox = SNew(SComboButton)
					.OnGetMenuContent(EditorPtr.Get(), &FAssetEditor_Dialogue::OnGetDebuggerActorsMenu )
					.ButtonContent()
					[
						SNew(STextBlock)
						.ToolTipText( LOCTEXT("SelectDebugExecutor", "Pick executor to debug") )
						.Text(EditorPtr.Get(), &FAssetEditor_Dialogue::GetDebuggerDesc )
					];

					
					ToolbarBuilder.AddToolBarButton(FDialogueDebuggerCommands::Get().StepToBegin);
					ToolbarBuilder.AddToolBarButton(FDialogueDebuggerCommands::Get().StepBack);
					ToolbarBuilder.AddToolBarButton(FDialogueDebuggerCommands::Get().StepForward);
					ToolbarBuilder.AddToolBarButton(FDialogueDebuggerCommands::Get().StepToEnd);

					ToolbarBuilder.AddSeparator();

					ToolbarBuilder.AddWidget(SelectionBox);

				}
			}
			ToolbarBuilder.EndSection();

		}
	};
	TWeakPtr<FAssetEditor_Dialogue> EditorPtr = SharedThis(this);
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar, EditorPtr)
	);
	
	AddToolbarExtender(ToolbarExtender);
}


void FAssetEditor_Dialogue::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	if (NewSelection.Num() > 0)
	{
		DetailsWidget->SetObjects(NewSelection.Array());
	}
	else
	{
		DetailsWidget->SetObject(AssetBeingEdited);
	}
}

void FAssetEditor_Dialogue::OnNodeDoubleClicked(UEdGraphNode* Node)
{	
	
}

void FAssetEditor_Dialogue::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (AssetBeingEdited == nullptr) 
	{
		return;
	}
	AssetBeingEdited->EdGraph->GetSchema()->ForceVisualizationCacheClear();

	if (UEdGraph_Dialogue* DialogueGraph = Cast<UEdGraph_Dialogue>(AssetBeingEdited->EdGraph))
	{
		DialogueGraph->OnAssetChanged();
	}
}


void FAssetEditor_Dialogue::RestoreDialogueGraph()
{
	UEdGraph_Dialogue* DialogueGraph = Cast<UEdGraph_Dialogue>(AssetBeingEdited->EdGraph);	

	if (DialogueGraph == nullptr)
	{
		DialogueGraph = Cast<UEdGraph_Dialogue>(FBlueprintEditorUtils::CreateNewGraph(AssetBeingEdited, NAME_None, UEdGraph_Dialogue::StaticClass(), UEdGraphSchema_Dialogue::StaticClass()));
		AssetBeingEdited->EdGraph = DialogueGraph;

		// Give the schema a chance to fill out any required nodes (like the results node)
		const UEdGraphSchema* Schema = DialogueGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*DialogueGraph);

		DialogueGraph->OnCreated();
	}
	else
	{
		DialogueGraph->OnLoaded();
	}

	DialogueGraph->Initialize();
}

void FAssetEditor_Dialogue::RebuildDialogueGraph()
{
	UEdGraph_Dialogue* EdGraph = Cast<UEdGraph_Dialogue>(AssetBeingEdited->EdGraph);
	check(EdGraph != nullptr);

	EdGraph->RebuildDialogueGraph();
}



TSharedPtr<SGraphEditor> FAssetEditor_Dialogue::GetGraphEditor() const
{
	return ViewportWidget;
}

FGraphPanelSelectionSet FAssetEditor_Dialogue::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GetGraphEditor();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	return CurrentSelection;
}

bool FAssetEditor_Dialogue::IsDebuggerReady() const
{
	return Debugger.IsValid() && Debugger->IsDebuggerReady();
}

FText FAssetEditor_Dialogue::GetDebuggerDesc() const
{
	return Debugger.IsValid() ? FText::FromString(Debugger->GetDebuggedInstanceDesc()) : FText::GetEmpty();
}

TSharedRef<class SWidget> FAssetEditor_Dialogue::OnGetDebuggerActorsMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);

	if (Debugger.IsValid())
	{
		TArray<UDialogueExecutorBase*> MatchingInstances;
		Debugger->GetMatchingInstances(MatchingInstances);

		// Fill the combo menu with presets of common screen resolutions
		for (int32 i = 0; i < MatchingInstances.Num(); i++)
		{
			if (MatchingInstances[i])
			{
				const FText Desc = FText::FromString(Debugger->DescribeInstance(*MatchingInstances[i]));
				TWeakObjectPtr<UDialogueExecutorBase> InstancePtr = MatchingInstances[i];

				FUIAction ItemAction(FExecuteAction::CreateSP(this, &FAssetEditor_Dialogue::OnDebuggerActorSelected, InstancePtr));
				MenuBuilder.AddMenuEntry(Desc, TAttribute<FText>(), FSlateIcon(), ItemAction);
			}
		}

		// Failsafe when no match
		if (MatchingInstances.Num() == 0)
		{
			const FText Desc = LOCTEXT("NoMatchForDebug", "Can't find matching executors");
			TWeakObjectPtr<UDialogueExecutorBase> InstancePtr;

			FUIAction ItemAction(FExecuteAction::CreateSP(this, &FAssetEditor_Dialogue::OnDebuggerActorSelected, InstancePtr));
			MenuBuilder.AddMenuEntry(Desc, TAttribute<FText>(), FSlateIcon(), ItemAction);
		}
	}

	return MenuBuilder.MakeWidget();
}

void FAssetEditor_Dialogue::OnDebuggerActorSelected(TWeakObjectPtr<UDialogueExecutorBase> InstanceToDebug)
{
	if (Debugger.IsValid())
	{
		Debugger->OnInstanceSelectedInDropdown(InstanceToDebug.Get());
	}
}

/*--------------------------------------------
 	Commands
 *--------------------------------------------*/

void FAssetEditor_Dialogue::SelectAllNodes()
{
	if (ViewportWidget.IsValid())
	{
		ViewportWidget->SelectAllNodes();
	}
}

bool FAssetEditor_Dialogue::CanSelectAllNodes()
{
	return true;
}

void FAssetEditor_Dialogue::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

	CurrentGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* EdNode = Cast<UEdGraphNode>(*NodeIt);
		if (EdNode == nullptr || !EdNode->CanUserDeleteNode())
		{
			continue;
		}

		EdNode->Modify();

		const UEdGraphSchema* Schema = EdNode->GetSchema();
		if (Schema != nullptr)
		{
			Schema->BreakNodeLinks(*EdNode);
		}

		EdNode->DestroyNode();
	}
}

bool FAssetEditor_Dialogue::CanDeleteNodes()
{
	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node != nullptr && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}


void FAssetEditor_Dialogue::CutSelectedNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	CopySelectedNodes();

	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();

	// Select duplicatable nodes
	CurrentGraphEditor->ClearSelectionSet();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}

	DeleteSelectedNodes();


	// Restore selection
	CurrentGraphEditor->ClearSelectionSet();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

bool FAssetEditor_Dialogue::CanCutNodes()
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FAssetEditor_Dialogue::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}
		Node->PrepareForCopying();
	}
	
	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FAssetEditor_Dialogue::CanCopyNodes()
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}

void FAssetEditor_Dialogue::PasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}
	const FVector2D& Location = CurrentGraphEditor->GetPasteLocation();

	UEdGraph* EdGraph = CurrentGraphEditor->GetCurrentGraph();

	{
		const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
		EdGraph->Modify();

		// Clear the selection set (newly pasted stuff will be selected)
		CurrentGraphEditor->ClearSelectionSet();

		// Grab the text to paste from the clipboard.
		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		// Import the nodes
		TSet<UEdGraphNode*> PastedNodes;
		FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);

		//Average position of nodes so we can move them while still maintaining relative distances to each other
		FVector2D AvgNodePosition(0.0f, 0.0f);

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			AvgNodePosition.X += Node->NodePosX;
			AvgNodePosition.Y += Node->NodePosY;
		}

		float InvNumNodes = 1.0f / float(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			CurrentGraphEditor->SetNodeSelection(Node, true);

			Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
			Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

			Node->SnapToGrid(16);

			// Give new node a different Guid from the old one
			Node->CreateNewGuid();
		}
	}

	// Update UI
	CurrentGraphEditor->NotifyGraphChanged();

	UObject* GraphOwner = EdGraph->GetOuter();
	if (GraphOwner)
	{
		GraphOwner->PostEditChange();
		GraphOwner->MarkPackageDirty();
	}
}


bool FAssetEditor_Dialogue::CanPasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FAssetEditor_Dialogue::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FAssetEditor_Dialogue::CanDuplicateNodes()
{
	return CanCopyNodes();
}


bool FAssetEditor_Dialogue::CanApplySnippet() const
{
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() == 1)
	{
		for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
		{
			UEdGraphNode_DialogueNode* Node = Cast<UEdGraphNode_DialogueNode>(*SelectedIter);
			if (Node)
			{
				FText Text = Node->Node.Text;
				if (Text.IsCultureInvariant() || !Text.IsFromStringTable())
				{
					return true;
				}
			}
		}
	}

	return false;
}

void FAssetEditor_Dialogue::ApplySelectedSnippet()
{
	if (SelectedSnippet.IsEmpty())
	{
		return;
	}

	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	if (SelectedNodes.Num() != 1)
	{
		return;
	}

	UEdGraphNode_DialogueNode* Node = nullptr;

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		Node = Cast<UEdGraphNode_DialogueNode>(*SelectedIter);
	}

	if (Node)
	{
		FText OriginalText = Node->Node.Text;

		if (OriginalText.IsCultureInvariant() || !OriginalText.IsFromStringTable())
		{
			const FScopedTransaction Transaction(FDialogueEditorCommands::Get().AddSnippet->GetDescription());


			FText NewText = FText::AsCultureInvariant(OriginalText.ToString() + SelectedSnippet);
			
			if (!OriginalText.IsCultureInvariant())
			{
				TOptional<FString> TextKey = FTextInspector::GetKey(OriginalText);
				TOptional<FString> TextNS = FTextInspector::GetNamespace(OriginalText);

				NewText = FText::ChangeKey(TextNS.Get(TEXT("")), TextKey.Get(TEXT("")), NewText);
			}			

			Node->Modify();

			Node->Node.Text = NewText;
		}
	}
}

void FAssetEditor_Dialogue::SelectSnippet(FString Snippet)
{
	SelectedSnippet = Snippet;
}

void FAssetEditor_Dialogue::ApplySnippet(FString Snippet)
{
	SelectSnippet(Snippet);
	ApplySelectedSnippet();
}

#undef LOCTEXT_NAMESPACE
