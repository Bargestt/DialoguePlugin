#pragma once

#include "Factories/Factory.h"
#include "AssetFactory_Dialogue.generated.h"

UCLASS(HideCategories = Object, MinimalAPI)
class UAssetFactory_Dialogue : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = DataAsset)
	TSubclassOf<class UDialogue> DialogueAssetClass;

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
