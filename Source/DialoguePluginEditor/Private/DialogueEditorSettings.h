

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Dialogue.h"
#include <Engine/DataTable.h>
#include "DialogueEditorSettings.generated.h"


/** */
USTRUCT(BlueprintInternalUseOnly)
struct FDialogueNodeTemplate : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FString DisplayName;

	UPROPERTY(EditAnywhere)
	FString Tooltip;

	UPROPERTY(EditAnywhere)
	FString DisplayCategory;

	UPROPERTY(EditAnywhere)
	FDialogueNode Node;
	
};


USTRUCT(BlueprintInternalUseOnly)
struct FDialogueNodeSnippet : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FString DisplayName;

	UPROPERTY(EditAnywhere)
	FString DisplayCategory;


	UPROPERTY(EditAnywhere)
	FString Value;
	
};


/** */
UENUM()
enum class EColorSource : uint8
{	
	/** Color from settings is used */
	None,
	/** Color from participant is used when available */
	Participant,
	/** Color from context is used when available */
	Context,
	/** Use Context first and Participant second */
	Both
};


/**
 * 
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "DialogueEditor"))
class UDialogueEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	static const UDialogueEditorSettings* Get() { return GetDefault<UDialogueEditorSettings>(); }
public:
	UDialogueEditorSettings();

	
	/** If empty, category misc is used */
	UPROPERTY(config, EditAnywhere, Category = Dialogue, meta = (ConfigRestartRequired = true))
	FText AssetCategory;


	/** When None, class picker window will show up */
	UPROPERTY(config, EditAnywhere, Category = Dialogue)
	TSubclassOf<UDialogue> DefaultAssetClass;

	/** Automatically set in newly created dialogue */
	UPROPERTY(config, EditAnywhere, Category = Dialogue)
	bool bUniformContext;

	/** Automatically set in newly created dialogue */
	UPROPERTY(config, EditAnywhere, Category = Dialogue)
	TSubclassOf<UDialogueNodeContext> DefaultNodeContext;

	UPROPERTY(config, EditAnywhere, Category = Dialogue)
	TArray<FName> CustomNodeTypes;


	UPROPERTY(config, EditAnywhere, Category = Dialogue)
	TArray<FName> StaticParticipantNames;

	UPROPERTY(config, EditAnywhere, Category = Dialogue, meta = (AllowedClasses = "DataTable"))
	TArray<FSoftObjectPath> NodeTemplates;
	

	UPROPERTY(config, EditAnywhere, Category = Snippets)
	TArray<FDialogueNodeSnippet> Snippets;

	UPROPERTY(config, EditAnywhere, Category = Snippets, meta = (AllowedClasses = "DataTable"))
	TArray<FSoftObjectPath> SnippetTables;



	UPROPERTY(config, EditAnywhere, Category = Details)
	bool bHideDefaultSound;

	UPROPERTY(config, EditAnywhere, Category = Details)
	bool bHideDefaultDialogueWave;

	/** 
	 * Allowed arguments:
	 * {ParticipantName}, {DialogueText}, {DialogueTextShort}, {ContextText}
	 * */
	//UPROPERTY(config, EditAnywhere, Category = Details)
	//FText NodeShortDescFormat;
		


	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	FLinearColor EntryColor;



	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	EColorSource NodeBodyColorSource;

	/** Default color */
	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	FLinearColor NodeColor;

	/** Custom node type default color */
	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	TMap<FName, FLinearColor> CustomTypeNodeColor;



	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	EColorSource NodeBorderColorSource;

	/** Default color */
	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	FLinearColor NodeBorderColor;

	/** Custom node type default color */
	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	TMap<FName, FLinearColor> CustomTypeNodeBorderColor;


	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	float NodeIconScale;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	float NodeIconHoverScale;


	/** Limit line number in node body */
	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	int32 MaxPreviewTextLines;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	float NodeMaxBodySize;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	bool BodyJustifyLeft;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	bool ContextJustifyLeft;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Nodes")
	float PinSize;



	UPROPERTY(config, EditAnywhere, Category = "Graph: Wires")
	FLinearColor WireColor;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Wires")
	FLinearColor WireColor_Selected;


	UPROPERTY(config, EditAnywhere, Category = "Graph: Wires")
	FLinearColor ReturnWireColor;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Wires")
	FLinearColor ReturnWireColor_Selected;



	UPROPERTY(config, EditAnywhere, Category = "Graph: Wires")
	float WireThickness;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Wires")
	float WireThicknessScale_Selected;


	UPROPERTY(config, EditAnywhere, Category = "Graph: Wires")
	float ReturnWireThicknessScale;


	UPROPERTY(config, EditAnywhere, Category = "Graph: Debugger")
	FLinearColor DebuggerState_Active;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Debugger")
	FLinearColor DebuggerState_Completed;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Debugger")
	FLinearColor DebuggerState_EntryAllowed;

	UPROPERTY(config, EditAnywhere, Category = "Graph: Debugger")
	FLinearColor DebuggerState_EntryDenied;
};
