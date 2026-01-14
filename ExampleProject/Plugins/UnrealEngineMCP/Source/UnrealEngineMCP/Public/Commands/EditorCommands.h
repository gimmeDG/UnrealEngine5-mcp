#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "GameplayTagContainer.h"

// Forward declarations for World Partition
class UWorldPartition;
class FWorldPartitionActorDesc;
class FWorldPartitionActorDescInstance;

/**
 * Handles editor and actor commands (spawn, delete, transform, find, modify)
 * Supports World Partition for unloaded actor search and region loading
 */
class UNREALENGINEMCP_API FEditorCommands
{
public:
	FEditorCommands();
	~FEditorCommands();

	/**
	 * Handle editor/actor command
	 * Routes to specific handler based on command type
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// Editor command handlers
	TSharedPtr<FJsonObject> HandleSpawnActor(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListLevelActors(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeleteActor(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params);

	// Actor command handlers
	TSharedPtr<FJsonObject> HandleSearchActors(const TSharedPtr<FJsonObject>& Params);

	// Material commands
	TSharedPtr<FJsonObject> HandleApplyMaterialToActor(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetActorMaterialInfo(const TSharedPtr<FJsonObject>& Params);

	// Object search
	TSharedPtr<FJsonObject> HandleSearchAssets(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListFolderAssets(const TSharedPtr<FJsonObject>& Params);
	TArray<TSharedPtr<FJsonValue>> SearchClasses(const FString& SearchName, const FString& BaseClass, int32 Limit);

	// World Partition commands - search and load unloaded actors
	TSharedPtr<FJsonObject> HandleSearchActorsInRegion(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleLoadActorByGuid(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetRegionLoaded(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetWorldPartitionInfo(const TSharedPtr<FJsonObject>& Params);

	// Helpers
	void AddAssetTypeFilter(FARFilter& Filter, const FString& AssetType);
	TSharedPtr<FJsonObject> ActorDescInstanceToJson(const FWorldPartitionActorDescInstance* ActorDescInstance, bool bIsLoaded);
	UWorldPartition* GetWorldPartition();

	// GAS Tag commands
	TSharedPtr<FJsonObject> HandleListGameplayTags(const TSharedPtr<FJsonObject>& Params);

	// Level Instance commands
	TSharedPtr<FJsonObject> HandleListLevelInstances(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetLevelInstanceActors(const TSharedPtr<FJsonObject>& Params);
};