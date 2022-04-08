

#pragma once

#include "DialoguePluginEditor.h"


struct FDialogueParticipantRegistry : public TSharedFromThis<FDialogueParticipantRegistry>
{
	bool bIsInitialized;

	TArray<UClass*> NativeClasses;
	TMap<FName, FAssetData> RegisteredAssets;
	TMap<FName, FAssetData> RegisteredBlueprints;

	TMap<FName, FName> GeneratedClassToBlueprint;

	//Blueprints with parent in class chain that implements interface
	TMap<FName, TArray<FName>> ChildrenBlueprints;	

	bool bRelevantBlueprintChanging;
public:
	FSimpleMulticastDelegate OnListChanged;

public:
	FDialogueParticipantRegistry();
	virtual ~FDialogueParticipantRegistry();
	void Initialize();

	void OnBlueprintPreCompile(UBlueprint* Blueprint);
	void OnBlueprintCompiled();

	void OnAssetAdded(const FAssetData& AssetData);	
	void OnAssetRemoved(const FAssetData& AssetData);	

	TArray<FAssetData> GetAssets() const;
	TArray<FAssetData> GetBlueprints() const;
	TArray<UClass*> GetClasses() const;


	void RegisterAsset(const FAssetData& AssetData, bool bIsBlueprint);
	void UnregisterAsset(const FAssetData& AssetData);

	void GatherAssets();

	virtual void AddAsset(const FAssetData& AssetData, bool bTriggerNotify);	


	// Check without loading asset
	static bool DoesBlueprintAssetImplementsInterface(const FAssetData& AssetData, const FName InterfaceName);


};

