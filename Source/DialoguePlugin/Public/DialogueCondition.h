

#pragma once

#include "CoreMinimal.h"
#include "DialogueCondition.generated.h"


/**
 * Basic dialogue condition
 */
UCLASS(Abstract, BlueprintType, Blueprintable, editinlinenew, collapseCategories, meta = (ShowWorldContextPin))
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
	
	UFUNCTION(BlueprintNativeEvent, Category = Dialogue, meta = (DisplayName = "IsConditionMet"))
	bool BP_IsConditionMet(UObject* WorldContext) const;

	UFUNCTION(BlueprintCallable, Category = Dialogue)
	UDialogue* GetDialogue() const;
};


/** To meet condition all nested conditions must be met */
UCLASS(NotBlueprintable, meta = (DisplayName = "AND"))
class DIALOGUEPLUGIN_API UDialogueCondition_AND : public UDialogueCondition
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	TArray<UDialogueCondition*> Conditions;

	virtual bool IsConditionMet(UObject* WorldContext) const override;
};

/** To meet condition any nested conditions must be met */
UCLASS(NotBlueprintable, meta = (DisplayName = "OR"))
class DIALOGUEPLUGIN_API UDialogueCondition_OR : public UDialogueCondition
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	TArray<UDialogueCondition*> Conditions;

	virtual bool IsConditionMet(UObject* WorldContext) const override;
};