// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DialogueParticipantInterface.generated.h"

class UDialogue;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UDialogueParticipantInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DIALOGUEPLUGIN_API IDialogueParticipantInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	/** 
	 * Return unique participant key to identify this participant in dialogue 
	 * Used to generate participant name list
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Dialogue)
	FName GetParticipantKey() const;



	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Dialogue)
	FText GetParticipantName() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Dialogue)
	FLinearColor GetParticipantColor() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Dialogue)
	FSlateBrush GetParticipantIcon() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Dialogue)
	UObject* GetParticipantObject() const;



	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue|Event")
	void OnDialogueStarted(UObject* WorldContextObject, UDialogue* Dialogue, FName EntryPoint);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue|Event")
	void OnNodeFinished(UObject* WorldContextObject, UDialogue* Dialogue, int32 NodeId);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue|Event")
	void OnNodeEntered(UObject* WorldContextObject, UDialogue* Dialogue, int32 NodeId);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dialogue|Event")
	void OnNodeLeft(UObject* WorldContextObject, UDialogue* Dialogue, int32 NodeId);


protected:
	virtual FName GetParticipantKey_Implementation() const
	{
		return NAME_None;
	}

	virtual FText GetParticipantName_Implementation() const
	{
		return FText::GetEmpty();
	}

	virtual FLinearColor GetParticipantColor_Implementation() const
	{
		return FLinearColor::White;
	}

	virtual FSlateBrush GetParticipantIcon_Implementation() const
	{
		return FSlateNoResource();
	}

	virtual UObject* GetParticipantObject_Implementation() const
	{
		return nullptr;
	}


	virtual void OnDialogueStarted_Implementation(UObject* WorldContextObject, UDialogue* Dialogue, FName EntryPoint) { }
	virtual void OnNodeFinished_Implementation(UObject* WorldContextObject, UDialogue* Dialogue, int32 NodeId) { }
	virtual void OnNodeEntered_Implementation(UObject* WorldContextObject, UDialogue* Dialogue, int32 NodeId) { }
	virtual void OnNodeLeft_Implementation(UObject* WorldContextObject, UDialogue* Dialogue, int32 NodeId) { }

};
