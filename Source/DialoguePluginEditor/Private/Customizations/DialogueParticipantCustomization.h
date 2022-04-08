

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IPropertyHandle;

class FDialogueParticipantCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef< IPropertyTypeCustomization > MakeInstance()
	{
		return MakeShareable(new FDialogueParticipantCustomization);
	}
	
	//~ Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{

	}
	//~ End IPropertyTypeCustomization Interface

protected:
	void GenerateStrings(TArray<TSharedPtr<FString>>& Strings, TArray<TSharedPtr<SToolTip>>& Tooltips, TArray<bool>& Bools);
};

