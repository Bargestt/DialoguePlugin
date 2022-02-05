
#include "DialogueParticipantRegistry.h"

#include <AssetRegistryModule.h>
#include "DialogueParticipantInterface.h"
#include <UObject/CoreRedirects.h>



FDialogueParticipantRegistry::FDialogueParticipantRegistry()
{
	
}

FDialogueParticipantRegistry::~FDialogueParticipantRegistry()
{
	if(bIsInitialized)
	{
		if (FModuleManager::Get().IsModuleLoaded(AssetRegistryConstants::ModuleName))
		{
			FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
			AssetRegistry.Get().OnFilesLoaded().RemoveAll(this);
			AssetRegistry.Get().OnAssetAdded().RemoveAll(this);
			AssetRegistry.Get().OnAssetRemoved().RemoveAll(this);
		}

		if (GEditor)
		{
			GEditor->OnBlueprintPreCompile().RemoveAll(this);
			GEditor->OnBlueprintCompiled().RemoveAll(this);
		}
	}
	bIsInitialized = false;
}


void FDialogueParticipantRegistry::Initialize()
{
	if (!bIsInitialized)
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
		AssetRegistry.Get().OnFilesLoaded().AddSP(this, &FDialogueParticipantRegistry::GatherAssets);
		AssetRegistry.Get().OnAssetAdded().AddSP(this, &FDialogueParticipantRegistry::OnAssetAdded);
		AssetRegistry.Get().OnAssetRemoved().AddSP(this, &FDialogueParticipantRegistry::OnAssetRemoved);

		if (GEditor)
		{
			GEditor->OnBlueprintPreCompile().AddSP(this, &FDialogueParticipantRegistry::OnBlueprintPreCompile);
			GEditor->OnBlueprintCompiled().AddSP(this, &FDialogueParticipantRegistry::OnBlueprintCompiled);
		}

		bIsInitialized = true;
	}

	GatherAssets();
}

void FDialogueParticipantRegistry::OnBlueprintPreCompile(UBlueprint* Blueprint)
{
	if (Blueprint && 
		RegisteredBlueprints.Contains(Blueprint->GetFName()) ||
		(Blueprint->GeneratedClass && Blueprint->GeneratedClass->ImplementsInterface(UDialogueParticipantInterface::StaticClass()))
		)
	{
		bRelevantBlueprintChanging = true;
	}
}

void FDialogueParticipantRegistry::OnBlueprintCompiled()
{
	if (bRelevantBlueprintChanging)
	{
		GatherAssets();
	}
	bRelevantBlueprintChanging = false;
}

void FDialogueParticipantRegistry::OnAssetAdded(const FAssetData& AssetData)
{
	AddAsset(AssetData, true);
}

void FDialogueParticipantRegistry::OnAssetRemoved(const FAssetData& AssetData)
{
	UnregisterAsset(AssetData);
}

TArray<FAssetData> FDialogueParticipantRegistry::GetAssets() const
{
	TArray<FAssetData> Arr;
	RegisteredAssets.GenerateValueArray(Arr);	
	return Arr;
}

TArray<FAssetData> FDialogueParticipantRegistry::GetBlueprints() const
{
	TArray<FAssetData> Arr;
	RegisteredBlueprints.GenerateValueArray(Arr);
	return Arr;
}

TArray<UClass*> FDialogueParticipantRegistry::GetClasses() const
{
	TArray<UClass*> Classes = NativeClasses;

	TArray<FAssetData> Blueprints = GetBlueprints();
	for (const FAssetData& AssetData : Blueprints)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (Blueprint && Blueprint->GeneratedClass)
		{
			Classes.Add(Blueprint->GeneratedClass);
		}
	}

	return Classes;
}

void FDialogueParticipantRegistry::RegisterAsset(const FAssetData& AssetData, bool bIsBlueprint)
{
	if (bIsBlueprint)
	{
		RegisteredBlueprints.Add(AssetData.PackageName, AssetData);
		GeneratedClassToBlueprint.Add(AssetData.GetTagValueRef<FName>(FBlueprintTags::GeneratedClassPath), AssetData.PackageName);
	}
	else
	{
		RegisteredAssets.Add(AssetData.PackageName, AssetData);
	}
}

void FDialogueParticipantRegistry::UnregisterAsset(const FAssetData& AssetData)
{
	if (RegisteredAssets.Contains(AssetData.PackageName))
	{
		RegisteredAssets.Remove(AssetData.PackageName);		
		OnListChanged.Broadcast();
	}
	else if (RegisteredBlueprints.Contains(AssetData.PackageName))
	{
		RegisteredBlueprints.Remove(AssetData.PackageName);
		GeneratedClassToBlueprint.Remove(AssetData.GetTagValueRef<FName>(FBlueprintTags::GeneratedClassPath));

		TArray<FName> ChildrenPackages;
		ChildrenBlueprints.RemoveAndCopyValue(AssetData.PackageName, ChildrenPackages);
		for (FName ChildPackage : ChildrenPackages)
		{
			FAssetData ChildAssetData;
			RegisteredBlueprints.RemoveAndCopyValue(ChildPackage, ChildAssetData);
			GeneratedClassToBlueprint.Remove(ChildAssetData.GetTagValueRef<FName>(FBlueprintTags::GeneratedClassPath));
		}

		OnListChanged.Broadcast();
	}
}

PRAGMA_DISABLE_OPTIMIZATION
void FDialogueParticipantRegistry::GatherAssets()
{
	// prevent asset crunching during PIE
	if (GEditor && GEditor->PlayWorld)
	{
		return;
	}

	RegisteredAssets.Empty();
	RegisteredBlueprints.Empty();
	ChildrenBlueprints.Empty();

	if (NativeClasses.Num() == 0)
	{
		TArray<UClass*> DerivedClasses;
		GetDerivedClasses(UObject::StaticClass(), DerivedClasses);
		for (UClass* Class : DerivedClasses)
		{
			if (Class->ClassGeneratedBy == nullptr && Class->ImplementsInterface(UDialogueParticipantInterface::StaticClass()))
			{
				NativeClasses.Add(Class);
			}
		}
	}


	// retrieve all blueprint nodes
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	FARFilter Filter;
	Filter.ClassNames.Add(UDialogueParticipantInterface::StaticClass()->GetFName());
	Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssets(Filter, Assets);


	// Build TempInheritanceMap
	TMap<FName, FName> TempInheritanceMap;
	for (const FAssetData& AssetData : Assets)
	{
		const FName GeneratedClassPath = AssetData.GetTagValueRef<FName>(FBlueprintTags::GeneratedClassPath);
		const FName ParentClassPath = AssetData.GetTagValueRef<FName>(FBlueprintTags::ParentClassPath);
		if (!GeneratedClassPath.IsNone() && !ParentClassPath.IsNone())
		{
			TempInheritanceMap.Add(GeneratedClassPath, ParentClassPath);
			check(GeneratedClassPath != ParentClassPath);
		}
	}

	for (const FAssetData& AssetData : Assets)
	{
		AddAsset(AssetData, false);
	}

	// Collect assets that inherit from registered assets
	for (const FAssetData& AssetData : Assets)
	{
		FName ParentPackageName = NAME_None;
		FName ParentClassPath = AssetData.GetTagValueRef<FName>(FBlueprintTags::GeneratedClassPath);
		while (ParentClassPath != NAME_None && ParentPackageName.IsNone())
		{
			ParentClassPath = TempInheritanceMap.FindRef(ParentClassPath);			
			ParentPackageName = GeneratedClassToBlueprint.FindRef(ParentClassPath);					
		}

		if (!ParentPackageName.IsNone())
		{
			RegisterAsset(AssetData, true);
			ChildrenBlueprints.FindOrAdd(AssetData.PackageName).AddUnique(AssetData.PackageName);
		}
	}

	{
// 		TArray<FName> BlueprintClasses;
// 
// 		TArray<FAssetData> Blueprints = GetBlueprints();
// 		for (const FAssetData& BlueprintAssetData : Blueprints)
// 		{
// 			UBlueprint* BP = Cast<UBlueprint>(BlueprintAssetData.GetAsset());
// 			if (BP && BP->GeneratedClass)
// 			{
// 				BlueprintClasses.Add(BP->GeneratedClass->GetFName());
// 			}
// 		}
// 		TSet<FName> Derived;
// 		AssetRegistryModule.Get().GetDerivedClassNames(BlueprintClasses, TSet<FName>(), Derived);
// 
// 		for (const FAssetData& AssetData : Assets)
// 		{
// 			const FName GeneratedClassPath = AssetData.GetTagValueRef<FName>(FBlueprintTags::GeneratedClassPath);
// 
// 			const FString ClassObjectPath = FPackageName::ExportTextPathToObjectPath(GeneratedClassPath.ToString());
// 			const FString ClassName = FPackageName::ObjectPathToObjectName(ClassObjectPath);
// 			if (Derived.Contains(*ClassName))
// 			{
// 				RegisteredBlueprints.Add(AssetData.PackageName, AssetData);
// 			}
// 		}
	}
		

	OnListChanged.Broadcast();
}



void FDialogueParticipantRegistry::AddAsset(const FAssetData& AssetData, bool bTriggerNotify)
{
	if (RegisteredAssets.Contains(AssetData.PackageName) || RegisteredBlueprints.Contains(AssetData.PackageName))
	{
		return;
	}

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		return;
	}

	TArray<FName> AncestorClassNames;
	AssetRegistryModule.Get().GetAncestorClassNames(AssetData.AssetClass, AncestorClassNames);

	bool bChanged = false;
	if (AncestorClassNames.Contains(UBlueprintCore::StaticClass()->GetFName()))
	{
		FString NativeParentClassPath;
		AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeParentClassPath);
		if (!NativeParentClassPath.IsEmpty())
		{
			UObject* Outer = nullptr;
			ResolveName(Outer, NativeParentClassPath, false, false);
			UClass* NativeParentClass = FindObject<UClass>(ANY_PACKAGE, *NativeParentClassPath);
			if (NativeParentClass && NativeParentClass->ImplementsInterface(UDialogueParticipantInterface::StaticClass()))
			{
				RegisterAsset(AssetData, true);
				bChanged = true;
			}
		}

		if (!bChanged)
		{
			if (DoesBlueprintAssetImplementsInterface(AssetData, UDialogueParticipantInterface::StaticClass()->GetFName()))
			{
				RegisterAsset(AssetData, true);
				bChanged = true;
			}
			else
			{
				const FName ParentClassPath = AssetData.GetTagValueRef<FName>(FBlueprintTags::ParentClassPath);
				if (!ParentClassPath.IsNone() && GeneratedClassToBlueprint.Contains(ParentClassPath))
				{
					RegisterAsset(AssetData, true);
					ChildrenBlueprints.FindOrAdd(AssetData.PackageName).AddUnique(AssetData.PackageName);
					bChanged = true;
				}
			}
		}		
	}
	else
	{
		UClass* AssetClass = AssetData.GetClass();
		if (!AssetClass)
		{
			return;
		}

		UObject* Asset = AssetData.GetAsset();
		if (Asset && Asset->Implements<UDialogueParticipantInterface>())
		{
			RegisterAsset(AssetData, false);
			bChanged = true;
		}
	}

	if (bChanged && bTriggerNotify)
	{
		OnListChanged.Broadcast();
	}
}
PRAGMA_ENABLE_OPTIMIZATION



bool FDialogueParticipantRegistry::DoesBlueprintAssetImplementsInterface(const FAssetData& AssetData, const FName InterfaceName)
{
	bool bMatchesInterface = false;

	const FString ImplementedInterfaces = AssetData.GetTagValueRef<FString>(FBlueprintTags::ImplementedInterfaces);
	if (!ImplementedInterfaces.IsEmpty())
	{
		FString FullInterface;
		FString RemainingString;
		FString InterfacePath;
		FString CurrentString = *ImplementedInterfaces;
		while (CurrentString.Split(TEXT(","), &FullInterface, &RemainingString) && !bMatchesInterface)
		{
			if (!CurrentString.StartsWith(TEXT("Graphs=(")))
			{
				if (FullInterface.Split(TEXT("\""), &CurrentString, &InterfacePath, ESearchCase::CaseSensitive))
				{
					// The interface paths in metadata end with "', so remove those
					InterfacePath.RemoveFromEnd(TEXT("\"'"));

					FCoreRedirectObjectName ResolvedInterfaceName = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class, FCoreRedirectObjectName(InterfacePath));
										
					bMatchesInterface |= ResolvedInterfaceName.ObjectName == InterfaceName;
				}
			}

			CurrentString = RemainingString;
		}
	}

	return bMatchesInterface;
}
