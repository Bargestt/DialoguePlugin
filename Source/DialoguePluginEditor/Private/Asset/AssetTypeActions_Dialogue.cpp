

#include "Asset/AssetTypeActions_Dialogue.h"

#include "Dialogue.h"

#include "Toolkits/IToolkit.h"
#include "Editor/AssetEditor_Dialogue.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_Dialogue"

FAssetTypeActions_Dialogue::FAssetTypeActions_Dialogue(EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{

}

FText FAssetTypeActions_Dialogue::GetName() const
{
	return LOCTEXT("AssetTypeActions_Dialogue", "Dialogue");
}

uint32 FAssetTypeActions_Dialogue::GetCategories()
{
	return AssetCategory;
}

FColor FAssetTypeActions_Dialogue::GetTypeColor() const
{
	return FColor(55, 55, 255);
}

UClass* FAssetTypeActions_Dialogue::GetSupportedClass() const
{
	return UDialogue::StaticClass();
}

void FAssetTypeActions_Dialogue::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	FAssetTypeActions_Base::OpenAssetEditor(InObjects, EditWithinLevelEditor);

	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UDialogue* DialogueAsset = Cast<UDialogue>(*ObjIt))
		{
			TSharedRef<FAssetEditor_Dialogue> NewEditor(new FAssetEditor_Dialogue());
			NewEditor->InitDialogueEditor(Mode, EditWithinLevelEditor, DialogueAsset);
		}
	}
}

#undef LOCTEXT_NAMESPACE
