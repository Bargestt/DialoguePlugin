// Copyright (C) Vasily Bulgakov. 2022. All Rights Reserved.



#pragma once

#include "AssetTypeActions_Base.h"
#include "Toolkits/IToolkitHost.h"

class FAssetTypeActions_Dialogue : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_Dialogue(EAssetTypeCategories::Type InAssetCategory);

	virtual FText GetName() const override;
	virtual uint32 GetCategories() override;
	virtual FColor GetTypeColor() const override;

	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

private:
	EAssetTypeCategories::Type AssetCategory;
};
