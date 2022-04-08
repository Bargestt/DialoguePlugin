// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



#pragma once

#include "CoreMinimal.h"
#include "DialogueCondition.generated.h"





/**
 * Basic dialogue condition
 */
UCLASS(Abstract, Blueprintable, BlueprintType, editinlinenew, collapseCategories, meta = (ShowWorldContextPin))
class DIALOGUEPLUGIN_API UDialogueCondition : public UObject
{
	GENERATED_BODY()

public:
	bool CheckCondition(UObject* WorldContext);

protected:
	virtual bool IsConditionMet(UObject* WorldContext) const
	{ 
		return true; 
	}
	
	/** Checked after native one. Both checks must succeed */
	UFUNCTION(BlueprintNativeEvent, Category = Dialogue, meta = (DisplayName = "IsConditionMet"))
	bool BP_IsConditionMet(UObject* WorldContext) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	UDialogue* GetDialogue() const;
};


/** To meet condition all nested conditions must be met */
UCLASS(NotBlueprintable, meta = (DisplayName = ".OR"))
class DIALOGUEPLUGIN_API UDialogueCondition_AND : public UDialogueCondition
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	TArray<UDialogueCondition*> Conditions;

	virtual bool IsConditionMet(UObject* WorldContext) const override;
};

/** To meet condition any nested conditions must be met */
UCLASS(NotBlueprintable, meta = (DisplayName = ".AND"))
class DIALOGUEPLUGIN_API UDialogueCondition_OR : public UDialogueCondition
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	TArray<UDialogueCondition*> Conditions;

	virtual bool IsConditionMet(UObject* WorldContext) const override;
};

/** To meet condition both nested conditions must return same/different result */
UCLASS(NotBlueprintable, meta = (DisplayName = ".Equal/NotEqual"))
class DIALOGUEPLUGIN_API UDialogueCondition_Equality : public UDialogueCondition
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCheckEqual = true;

	/** Invalid object defaults to false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	UDialogueCondition* A;

	/** Invalid object defaults to false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	UDialogueCondition* B;

	virtual bool IsConditionMet(UObject* WorldContext) const override;
};