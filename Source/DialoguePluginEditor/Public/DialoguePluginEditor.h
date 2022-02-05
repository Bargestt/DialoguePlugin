// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <Modules/ModuleInterface.h>


class IDialoguePluginEditor : public IModuleInterface
{
public:
	/** Assets that implement UDialogueParticipantInterface */
	virtual TArray<FAssetData> GetDialogueAssets() = 0;

	/** Blueprints that implement UDialogueParticipantInterface */
	virtual TArray<FAssetData> GetDialogueBlueprints() = 0;

	/** Classes that implement UDialogueParticipantInterface */
	virtual TArray<UClass*> GetDialogueClasses() = 0;
};
