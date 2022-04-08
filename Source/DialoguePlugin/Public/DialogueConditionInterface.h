

#pragma once

#include "CoreMinimal.h"
#include "DialogueConditionInterface.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UDialogueConditionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class DIALOGUEPLUGIN_API IDialogueConditionInterface
{
	GENERATED_BODY()	
public:
	/** Checked first before BP event */
	virtual bool IsConditionMet(UObject* WorldContext, UObject* Dialogue, int32 NodeId) const
	{
		return true;
	}

	/** Checked after native, skipped if native fails */
	UFUNCTION(BlueprintImplementableEvent, Category = Dialogue, meta = (DisplayName = "IsConditionMet"))
	void ReceiveIsConditionMet(UObject* WorldContext, UObject* Dialogue, int32 NodeId, bool& bIsMet) const;

	/** Used to display what condition does */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Dialogue)
	FString GetConditionTitle(bool bFullTitle) const;
	virtual FString GetConditionTitle_Implementation(bool bFullTitle) const;

	/** Static condition are run on CDO */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Dialogue)
	bool IsConditionStatic() const;
	virtual	bool IsConditionStatic_Implementation() const
	{
		return false;
	}



	static FString GetTitleSafe(UObject* Object, bool bFullTitle)
	{
		if (!Object)
		{
			return TEXT("None");
		}

		if (!Object->Implements<UDialogueConditionInterface>())
		{
			return TEXT("Bad object: ") + Object->GetName();
		}

		return IDialogueConditionInterface::Execute_GetConditionTitle(Object, bFullTitle);
	}
};


/** 
 * Condition selector
 * Condition can be instanced or run on CDO
 */
USTRUCT(BlueprintType)
struct DIALOGUEPLUGIN_API FDialogueCondition
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
	UObject* Condition;

	FDialogueCondition()
		: Condition(nullptr)
	{ }

	bool CheckCondition(UObject* WorldContext, UObject* Dialogue, int32 NodeId) const
	{
		bool bResult = true;
		if (Condition)
		{
			if (IDialogueConditionInterface* NativeCheck = Cast<IDialogueConditionInterface>(Condition))
			{
				bResult = NativeCheck->IsConditionMet(WorldContext, Dialogue, NodeId);
			}			

			if (bResult && (Condition->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) || !Condition->GetClass()->HasAnyClassFlags(CLASS_Native)))
			{
				IDialogueConditionInterface::Execute_ReceiveIsConditionMet(Condition, WorldContext, Dialogue, NodeId, bResult);
			}
		}
		
		return bResult;
	}

	bool IsValid() const 
	{ 
		return Condition != NULL; 
	}	
};

/** Array with promoted elements to reduce nesting */
USTRUCT(BlueprintType)
struct DIALOGUEPLUGIN_API FDialogueConditionArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDialogueCondition> Array;

	FORCEINLINE int32 Num() const 
	{ 
		return Array.Num(); 
	}
	FORCEINLINE FDialogueCondition& operator[](int32 Index)
	{
		return Array[Index];
	}
	FORCEINLINE const FDialogueCondition& operator[](int32 Index) const
	{
		return Array[Index];
	}
};


/**
 * Basic dialogue condition
 * Is optional, any object that implements IDialogueConditionInterface can be used as condition
 */
UCLASS(Abstract, Blueprintable, BlueprintType, editinlinenew, collapseCategories, meta = (ShowWorldContextPin))
class DIALOGUEPLUGIN_API UDialogueConditionBase : public UObject, public IDialogueConditionInterface
{
	GENERATED_BODY()
};


/*--------------------------------------------
 	Default boolean operators
 *--------------------------------------------*/


/** To meet condition all nested conditions must be met */
UCLASS(NotBlueprintable, meta = (DisplayName = ".AND"))
class DIALOGUEPLUGIN_API UDialogueCondition_AND : public UDialogueConditionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDialogueConditionArray Conditions;

	virtual bool IsConditionMet(UObject* WorldContext, UObject* Dialogue, int32 NodeId) const override;
	virtual FString GetConditionTitle_Implementation(bool bFullTitle) const override;
	virtual	bool IsConditionStatic_Implementation() const { return false; }
};

/** To meet condition any nested conditions must be met */
UCLASS(NotBlueprintable, meta = (DisplayName = ".OR"))
class DIALOGUEPLUGIN_API UDialogueCondition_OR : public UDialogueConditionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDialogueConditionArray Conditions;

	virtual bool IsConditionMet(UObject* WorldContext, UObject* Dialogue, int32 NodeId) const override;
	virtual FString GetConditionTitle_Implementation(bool bFullTitle) const override;
	virtual	bool IsConditionStatic_Implementation() const { return false; }
};

/** To meet condition both nested conditions must return same/different result */
UCLASS(NotBlueprintable, meta = (DisplayName = ".Equal/NotEqual"))
class DIALOGUEPLUGIN_API UDialogueCondition_Equality : public UDialogueConditionBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCheckEqual = true;

	/** Invalid object defaults to false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDialogueCondition A;

	/** Invalid object defaults to false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDialogueCondition B;

	virtual bool IsConditionMet(UObject* WorldContext, UObject* Dialogue, int32 NodeId) const override;
	virtual FString GetConditionTitle_Implementation(bool bFullTitle) const override;
	virtual	bool IsConditionStatic_Implementation() const { return false; }
};