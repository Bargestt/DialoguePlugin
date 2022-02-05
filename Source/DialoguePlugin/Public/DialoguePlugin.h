// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DialoguePlugin.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDialogue, Log, All);


class FDialoguePlugin : public IModuleInterface
{
};


UCLASS()
class UDialogueUtilityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	/**  
	 * Create and setup dialogue executor. Dialogue must be started explicitly
	 * @param bDeferInitialization When true initialization must be done manually
	 * @param Owner Outer object of executor, used as world context
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue", meta = (DynamicOutputParam = OutExecutor, DeterminesOutputType = Class))
	static void CreateDialogueExecutor(TSubclassOf<UDialogueExecutorBase> Class, UObject* Owner, UDialogue* Dialogue, bool bDeferInitialization, UDialogueExecutorBase*& OutExecutor);
		

};
