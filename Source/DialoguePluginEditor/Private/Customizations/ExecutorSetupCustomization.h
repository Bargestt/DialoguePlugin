#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IPropertyHandle;
class UDialogue;

class FExecutorSetupCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef< IPropertyTypeCustomization > MakeInstance()
	{
		return MakeShareable(new FExecutorSetupCustomization);
	}
	
	//~ Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	//~ End IPropertyTypeCustomization Interface

protected:
	void GenerateStrings(TArray<TSharedPtr<FString>>& Strings, TArray<TSharedPtr<SToolTip>>& Tooltips, TArray<bool>& Bools);

	void DialogueChanged();

	FReply OnClickRefresh();
	FReply OnClickReset();
	EVisibility GetParticipantsVisibility() const;
	FText GetMessage() const;
	FText GetTooltip() const;

	TSharedPtr<IPropertyHandle> DialoguePropertyHandle;

	struct FExecutorSetup* Setup;

	FString Message;
	FString MessageTooltip;
};

