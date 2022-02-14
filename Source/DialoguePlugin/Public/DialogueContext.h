// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DialogueContext.generated.h"


/**
 * Base context class for each node
 */
UCLASS(Abstract, Blueprintable, BlueprintType, editinlinenew, meta = (ShowWorldContextPin))
class DIALOGUEPLUGIN_API UDialogueNodeContext : public UObject
{
	GENERATED_BODY()

	friend struct FDialogueNode;
private:
	UPROPERTY()
	int32 NodeId;

public:

	UFUNCTION(BlueprintNativeEvent, Category = Dialogue)
	FString GetContextDescripton() const;
	virtual FString GetContextDescripton_Implementation() const { return TEXT(""); }
	
	UFUNCTION(BlueprintNativeEvent, Category = Dialogue)
	bool GetOverrideTitle(FString& OutTitle) const;
	virtual bool GetOverrideTitle_Implementation(FString& OutTitle) const { return false; }

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Dialogue)
	bool GetOverrideIcon(FSlateBrush& OutIcon) const;
	virtual bool GetOverrideIcon_Implementation(FSlateBrush& OutIcon) const { return false; }

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Dialogue)
	bool GetOverrideNodeColor(FLinearColor& OutColor) const;
	virtual bool GetOverrideNodeColor_Implementation(FLinearColor& OutColor) const	{ return false; }


	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue|Event")
	bool CanEnterNode(UObject* WorldContextObject, int32 FromNode) const;
	virtual bool CanEnterNode_Implementation(UObject* WorldContextObject, int32 FromNode) const
	{
		return true;
	}


	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue|Event")
	void OnDialogueStarted(UObject* WorldContextObject, FName EntryPoint);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue|Event")
	void OnNodeFinished(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue|Event")
	void OnNodeEntered(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue|Event")
	void OnNodeLeft(UObject* WorldContextObject);

	virtual void OnDialogueStarted_Implementation(UObject* WorldContextObject, FName EntryPoint) { }
	virtual void OnNodeFinished_Implementation(UObject* WorldContextObject) { }
	virtual void OnNodeEntered_Implementation(UObject* WorldContextObject) { }
	virtual void OnNodeLeft_Implementation(UObject* WorldContextObject) { }

public:
	UFUNCTION(BlueprintCallable, Category = Dialogue)
	int32 GetNodeId() const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	UDialogue* GetDialogue() const;
};