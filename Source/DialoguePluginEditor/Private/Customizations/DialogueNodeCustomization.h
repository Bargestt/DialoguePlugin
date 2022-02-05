#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IPropertyHandle;
class UEdGraphNode_DialogueNode;

class FDialogueNodeCustomization : public IDetailCustomization
{		
	
public:
	static TSharedRef< IDetailCustomization > MakeInstance()
	{
		return MakeShared<FDialogueNodeCustomization>();
	}
	
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	void OnSetClass(const UClass* Class);
	const UClass* GetSelectedClass() const;

	void GenerateStrings(TArray<TSharedPtr<FString>>& Strings, TArray<TSharedPtr<SToolTip>>& Tooltips, TArray<bool>& Bools);
	void ResetType();

	EVisibility GetResetTypeVisibility() const;

private:
	TSharedPtr<IPropertyHandle> NodeTypeProperty;
	TWeakObjectPtr<class UDialogue> Dialogue;

	TArray<TWeakObjectPtr<UObject>> Objects;
};