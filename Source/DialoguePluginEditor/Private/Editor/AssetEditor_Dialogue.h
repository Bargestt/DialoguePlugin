// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "Editor/EditorWidgets/Public/ITransportControl.h"
#include "EditorUndoClient.h"
#include "Framework/Commands/UICommandList.h"

#include "Dialogue.h"

class UEdGraph_Dialogue;


class FAssetEditor_Dialogue : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	FAssetEditor_Dialogue();
	virtual ~FAssetEditor_Dialogue();

	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	// End of IToolkit interface

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient Interface

	//~ Begin FAssetEditorToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual void SaveAsset_Execute() override;
	//~ End FAssetEditorToolkit Interface
	

public:
	void InitDialogueEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UDialogue* InitDialogue);
	UDialogue* GetAssetBeingEdited() const { return AssetBeingEdited; }
	UEdGraph_Dialogue* GetGraph() const;

protected:
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Snippets(const FSpawnTabArgs& Args);

private:
	void CreateGraphCommandList();

	void BindCommands();
	void BindDebuggerCommands();

	void ExtendToolbar();

	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);
	void OnNodeDoubleClicked(UEdGraphNode* Node);

	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

	void RestoreDialogueGraph();
	void RebuildDialogueGraph();
	
	TSharedPtr<SGraphEditor> GetGraphEditor() const;
	FGraphPanelSelectionSet GetSelectedNodes() const;

	bool IsDebuggerReady() const;
	FText GetDebuggerDesc() const;
	TSharedRef<class SWidget> OnGetDebuggerActorsMenu();
	void OnDebuggerActorSelected(TWeakObjectPtr<class UDialogueExecutorBase> InstanceToDebug);

	// Commands
	void SelectAllNodes();
	bool CanSelectAllNodes();

	void DeleteSelectedNodes();
	bool CanDeleteNodes();

	void CutSelectedNodes();
	bool CanCutNodes();

	void CopySelectedNodes();
	bool CanCopyNodes();

	void PasteNodes();
	bool CanPasteNodes();

	void DuplicateNodes();
	bool CanDuplicateNodes();

	bool CanApplySnippet() const;
	void ApplySelectedSnippet();

	void SelectSnippet(FString Snippet);
	void ApplySnippet(FString Snippet);

private:
	UDialogue* AssetBeingEdited;

	TSharedPtr<SGraphEditor> ViewportWidget;
	TSharedPtr<class IDetailsView> DetailsWidget;

	TSharedPtr<class FDialogueDebugger> Debugger;

	TSharedPtr<FUICommandList> GraphEditorCommands;

	FString SelectedSnippet;
};