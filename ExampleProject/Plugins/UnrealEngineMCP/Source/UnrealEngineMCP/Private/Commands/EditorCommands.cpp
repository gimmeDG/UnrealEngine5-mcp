#include "Commands/EditorCommands.h"
#include "Commands/CommonUtils.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Editor.h"
#include "EditorActorFolders.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Misc/PackageName.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/ActorComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Factories/MaterialFactoryNew.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
// Additional actor types
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
// World Partition support (UE5+)
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionHelpers.h"
#include "WorldPartition/WorldPartitionActorDescInstance.h"
// GameplayTags support
#include "GameplayTagsManager.h"
#include "GameplayTagContainer.h"
// Level Instance support
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceInterface.h"
#include "LevelInstance/LevelInstanceEditorInstanceActor.h"
#include "Interfaces/IPluginManager.h"

FEditorCommands::FEditorCommands()
{
}

FEditorCommands::~FEditorCommands()
{
}

TSharedPtr<FJsonObject> FEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("spawn_actor"))
	{
		return HandleSpawnActor(Params);
	}
	else if (CommandType == TEXT("list_level_actors"))
	{
		return HandleListLevelActors(Params);
	}
	else if (CommandType == TEXT("delete_actor"))
	{
		return HandleDeleteActor(Params);
	}
	else if (CommandType == TEXT("set_actor_transform"))
	{
		return HandleSetActorTransform(Params);
	}
	else if (CommandType == TEXT("get_actor_properties"))
	{
		return HandleGetActorProperties(Params);
	}
	else if (CommandType == TEXT("set_actor_property"))
	{
		return HandleSetActorProperty(Params);
	}
	else if (CommandType == TEXT("spawn_blueprint_actor"))
	{
		return HandleSpawnBlueprintActor(Params);
	}
	else if (CommandType == TEXT("create_material"))
	{
		return HandleCreateMaterial(Params);
	}
	else if (CommandType == TEXT("search_actors"))
	{
		return HandleSearchActors(Params);
	}
	// Material commands
	else if (CommandType == TEXT("apply_material_to_actor"))
	{
		return HandleApplyMaterialToActor(Params);
	}
	else if (CommandType == TEXT("get_actor_material_info"))
	{
		return HandleGetActorMaterialInfo(Params);
	}
	else if (CommandType == TEXT("search_assets"))
	{
		return HandleSearchAssets(Params);
	}
	else if (CommandType == TEXT("list_folder_assets"))
	{
		return HandleListFolderAssets(Params);
	}
	// World Partition commands
	else if (CommandType == TEXT("search_actors_in_region"))
	{
		return HandleSearchActorsInRegion(Params);
	}
	else if (CommandType == TEXT("load_actor_by_guid"))
	{
		return HandleLoadActorByGuid(Params);
	}
	else if (CommandType == TEXT("set_region_loaded"))
	{
		return HandleSetRegionLoaded(Params);
	}
	else if (CommandType == TEXT("get_world_partition_info"))
	{
		return HandleGetWorldPartitionInfo(Params);
	}
	// GAS Tag commands
	else if (CommandType == TEXT("list_gameplay_tags"))
	{
		return HandleListGameplayTags(Params);
	}
	// Level Instance commands
	else if (CommandType == TEXT("list_level_instances"))
	{
		return HandleListLevelInstances(Params);
	}
	else if (CommandType == TEXT("get_level_instance_actors"))
	{
		return HandleGetLevelInstanceActors(Params);
	}

	return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters with validation
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString ActorType;
	if (!Params->TryGetStringField(TEXT("type"), ActorType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
	}

	// Get optional transform parameters
	FVector Location(0.0f, 0.0f, 0.0f);
	FRotator Rotation(0.0f, 0.0f, 0.0f);
	FVector Scale(1.0f, 1.0f, 1.0f);

	if (Params->HasField(TEXT("location")))
	{
		Location = FCommonUtils::GetVectorFromJson(Params, TEXT("location"));
	}
	if (Params->HasField(TEXT("rotation")))
	{
		Rotation = FCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
	}
	if (Params->HasField(TEXT("scale")))
	{
		Scale = FCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
	}

	// Get optional static_mesh parameter for custom mesh
	FString StaticMeshPath;
	bool bHasCustomMesh = Params->TryGetStringField(TEXT("static_mesh"), StaticMeshPath);

	// Get world
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
	}

	// Check if an actor with this name already exists (O(1) lookup using FindObject)
	// This is much faster than iterating all actors when there are tens of thousands
	AActor* ExistingActor = FindObject<AActor>(World->GetCurrentLevel(), *ActorName);
	if (ExistingActor)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
	}

	// Spawn actor based on type
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(*ActorName);

	AActor* SpawnedActor = nullptr;

	// Normalize actor type to uppercase for case-insensitive comparison
	FString ActorTypeUpper = ActorType.ToUpper();

	// Helper lambda to set static mesh on StaticMeshActor
	auto SetBasicShapeMesh = [](AStaticMeshActor* MeshActor, const TCHAR* MeshPath) -> bool
	{
		if (!MeshActor) return false;

		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
		if (Mesh)
		{
			UStaticMeshComponent* MeshComponent = MeshActor->GetStaticMeshComponent();
			if (MeshComponent)
			{
				MeshComponent->SetStaticMesh(Mesh);
				return true;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("FEditorCommands::HandleSpawnActor: Failed to load mesh: %s"), MeshPath);
		return false;
	};

	if (ActorTypeUpper == TEXT("CUBE") || ActorTypeUpper == TEXT("STATICMESHACTOR"))
	{
		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
		// Use custom mesh if provided, otherwise default to Cube
		FString MeshPath = bHasCustomMesh ? StaticMeshPath : TEXT("/Engine/BasicShapes/Cube.Cube");
		SetBasicShapeMesh(MeshActor, *MeshPath);
		SpawnedActor = MeshActor;
	}
	else if (ActorTypeUpper == TEXT("SPHERE"))
	{
		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
		// Use custom mesh if provided, otherwise default to Sphere
		FString MeshPath = bHasCustomMesh ? StaticMeshPath : TEXT("/Engine/BasicShapes/Sphere.Sphere");
		SetBasicShapeMesh(MeshActor, *MeshPath);
		SpawnedActor = MeshActor;
	}
	else if (ActorTypeUpper == TEXT("CYLINDER"))
	{
		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
		// Use custom mesh if provided, otherwise default to Cylinder
		FString MeshPath = bHasCustomMesh ? StaticMeshPath : TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
		SetBasicShapeMesh(MeshActor, *MeshPath);
		SpawnedActor = MeshActor;
	}
	else if (ActorTypeUpper == TEXT("CONE"))
	{
		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
		// Use custom mesh if provided, otherwise default to Cone
		FString MeshPath = bHasCustomMesh ? StaticMeshPath : TEXT("/Engine/BasicShapes/Cone.Cone");
		SetBasicShapeMesh(MeshActor, *MeshPath);
		SpawnedActor = MeshActor;
	}
	else if (ActorTypeUpper == TEXT("PLANE"))
	{
		AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
		// Use custom mesh if provided, otherwise default to Plane
		FString MeshPath = bHasCustomMesh ? StaticMeshPath : TEXT("/Engine/BasicShapes/Plane.Plane");
		SetBasicShapeMesh(MeshActor, *MeshPath);
		SpawnedActor = MeshActor;
	}
	else if (ActorTypeUpper == TEXT("POINTLIGHT"))
	{
		SpawnedActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
	}
	else if (ActorTypeUpper == TEXT("SPOTLIGHT"))
	{
		SpawnedActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
	}
	else if (ActorTypeUpper == TEXT("DIRECTIONALLIGHT"))
	{
		SpawnedActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
	}
	else if (ActorTypeUpper == TEXT("CAMERAACTOR") || ActorTypeUpper == TEXT("CAMERA"))
	{
		SpawnedActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
	}
	else
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Unknown actor type: %s. Supported types: CUBE, SPHERE, CYLINDER, CONE, PLANE, STATICMESHACTOR, POINTLIGHT, SPOTLIGHT, DIRECTIONALLIGHT, CAMERAACTOR"),
			*ActorType));
	}

	if (SpawnedActor)
	{
		// Apply scale (SpawnActor only takes location and rotation)
		FTransform Transform = SpawnedActor->GetTransform();
		Transform.SetScale3D(Scale);
		SpawnedActor->SetActorTransform(Transform);

		// Set the Actor Label (display name in Outliner) to match the requested name
		SpawnedActor->SetActorLabel(ActorName);

		UE_LOG(LogTemp, Display, TEXT("FEditorCommands::HandleSpawnActor: Spawned actor '%s' (label: '%s', type: %s) at %s with scale %s"),
			*SpawnedActor->GetName(), *ActorName, *ActorType, *Location.ToString(), *Scale.ToString());

		// Return detailed actor info
		return FCommonUtils::ActorToJsonObject(SpawnedActor, true);
	}

	return FCommonUtils::CreateErrorResponse(TEXT("Failed to spawn actor"));
}

TSharedPtr<FJsonObject> FEditorCommands::HandleListLevelActors(const TSharedPtr<FJsonObject>& Params)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
	}

	bool bIncludeLevelInstances = true;
	Params->TryGetBoolField(TEXT("include_level_instances"), bIncludeLevelInstances);

	TArray<TSharedPtr<FJsonValue>> ActorsArray;
	TArray<TSharedPtr<FJsonValue>> LevelInstancesArray;

	FCommonUtils::ForEachActorInWorld(World, [&](AActor* Actor, ALevelInstance* OwningLI) -> bool
	{
		if (!Actor || Actor->IsHidden())
		{
			return true;
		}

		TSharedPtr<FJsonObject> ActorInfo = MakeShared<FJsonObject>();
		ActorInfo->SetStringField(TEXT("name"), Actor->GetName());
		ActorInfo->SetStringField(TEXT("label"), Actor->GetActorLabel());
		ActorInfo->SetStringField(TEXT("class"), Actor->GetClass()->GetName());

		FVector Location = Actor->GetActorLocation();
		TArray<TSharedPtr<FJsonValue>> LocationArray;
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
		ActorInfo->SetArrayField(TEXT("location"), LocationArray);

		if (OwningLI)
		{
			ActorInfo->SetStringField(TEXT("level_instance"), OwningLI->GetName());
			ActorInfo->SetStringField(TEXT("level_instance_label"), OwningLI->GetActorLabel());
		}

		ActorsArray.Add(MakeShared<FJsonValueObject>(ActorInfo));
		return true;
	}, bIncludeLevelInstances);

	// Collect Level Instance info
	TArray<ALevelInstance*> LevelInstances = FCommonUtils::GetAllLevelInstances(World);
	for (ALevelInstance* LI : LevelInstances)
	{
		TSharedPtr<FJsonObject> LIInfo = MakeShared<FJsonObject>();
		LIInfo->SetStringField(TEXT("name"), LI->GetName());
		LIInfo->SetStringField(TEXT("label"), LI->GetActorLabel());
		LIInfo->SetStringField(TEXT("class"), LI->GetClass()->GetName());

		if (TSoftObjectPtr<UWorld> WorldAsset = LI->GetWorldAsset())
		{
			LIInfo->SetStringField(TEXT("world_asset"), WorldAsset.ToString());
		}

		FVector LILocation = LI->GetActorLocation();
		TArray<TSharedPtr<FJsonValue>> LILocationArray;
		LILocationArray.Add(MakeShared<FJsonValueNumber>(LILocation.X));
		LILocationArray.Add(MakeShared<FJsonValueNumber>(LILocation.Y));
		LILocationArray.Add(MakeShared<FJsonValueNumber>(LILocation.Z));
		LIInfo->SetArrayField(TEXT("location"), LILocationArray);

		ULevel* LoadedLevel = FCommonUtils::GetLevelInstanceLoadedLevel(LI);
		LIInfo->SetBoolField(TEXT("is_loaded"), LoadedLevel != nullptr);

		if (LoadedLevel)
		{
			int32 ActorCount = 0;
			for (AActor* A : LoadedLevel->Actors)
			{
				if (A && !A->IsA<ALevelInstanceEditorInstanceActor>())
				{
					ActorCount++;
				}
			}
			LIInfo->SetNumberField(TEXT("actor_count"), ActorCount);
		}

		LevelInstancesArray.Add(MakeShared<FJsonValueObject>(LIInfo));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetArrayField(TEXT("actors"), ActorsArray);
	ResultObj->SetArrayField(TEXT("level_instances"), LevelInstancesArray);
	ResultObj->SetNumberField(TEXT("actor_count"), ActorsArray.Num());
	ResultObj->SetNumberField(TEXT("level_instance_count"), LevelInstancesArray.Num());

	return ResultObj;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
	}

	// Find actor with auto-load support for World Partition
	bool bWasAutoLoaded = false;
	AActor* FoundActor = FCommonUtils::FindActorByNameWithAutoLoad(World, ActorName, bWasAutoLoaded);

	if (!FoundActor)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor '%s' not found (searched both loaded actors and World Partition)"), *ActorName));
	}

	World->DestroyActor(FoundActor);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	if (bWasAutoLoaded)
	{
		ResultObj->SetBoolField(TEXT("was_auto_loaded_from_world_partition"), true);
	}
	UE_LOG(LogTemp, Display, TEXT("FEditorCommands::HandleDeleteActor: Deleted actor '%s'%s"), *ActorName,
		bWasAutoLoaded ? TEXT(" (auto-loaded from World Partition)") : TEXT(""));

	return ResultObj;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
	// Get actor name
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	// Find the actor with auto-load support for World Partition
	bool bWasAutoLoaded = false;
	AActor* TargetActor = FCommonUtils::FindActorByNameWithAutoLoad(GWorld, ActorName, bWasAutoLoaded);
	if (!TargetActor)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s (searched both loaded actors and World Partition)"), *ActorName));
	}

	// Get transform parameters
	FTransform NewTransform = TargetActor->GetTransform();

	if (Params->HasField(TEXT("location")))
	{
		NewTransform.SetLocation(FCommonUtils::GetVectorFromJson(Params, TEXT("location")));
	}
	if (Params->HasField(TEXT("rotation")))
	{
		NewTransform.SetRotation(FQuat(FCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
	}
	if (Params->HasField(TEXT("scale")))
	{
		NewTransform.SetScale3D(FCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
	}

	// Set the new transform
	TargetActor->SetActorTransform(NewTransform);

	// Return updated actor info with auto-load status
	TSharedPtr<FJsonObject> Result = FCommonUtils::ActorToJsonObject(TargetActor, true);
	if (bWasAutoLoaded)
	{
		Result->SetBoolField(TEXT("was_auto_loaded_from_world_partition"), true);
	}
	return Result;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
	// Get actor name
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	// Find the actor with auto-load support for World Partition
	bool bWasAutoLoaded = false;
	AActor* TargetActor = FCommonUtils::FindActorByNameWithAutoLoad(GWorld, ActorName, bWasAutoLoaded);
	if (!TargetActor)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s (searched both loaded actors and World Partition)"), *ActorName));
	}

	// Return detailed properties with auto-load status
	TSharedPtr<FJsonObject> Result = FCommonUtils::ActorToJsonObject(TargetActor, true);
	if (bWasAutoLoaded)
	{
		Result->SetBoolField(TEXT("was_auto_loaded_from_world_partition"), true);
	}
	return Result;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
	// Get actor name
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("name"), ActorName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	// Find the actor with auto-load support for World Partition
	bool bWasAutoLoaded = false;
	AActor* TargetActor = FCommonUtils::FindActorByNameWithAutoLoad(GWorld, ActorName, bWasAutoLoaded);
	if (!TargetActor)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s (searched both loaded actors and World Partition)"), *ActorName));
	}

	// Get property name
	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
	}

	// Get property value
	if (!Params->HasField(TEXT("property_value")))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
	}

	TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));

	// Set the property using our utility function
	FString ErrorMessage;
	if (FCommonUtils::SetObjectProperty(TargetActor, PropertyName, PropertyValue, ErrorMessage))
	{
		// Property set successfully
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("actor"), ActorName);
		ResultObj->SetStringField(TEXT("property"), PropertyName);
		ResultObj->SetBoolField(TEXT("success"), true);
		if (bWasAutoLoaded)
		{
			ResultObj->SetBoolField(TEXT("was_auto_loaded_from_world_partition"), true);
		}

		// Also include the full actor details
		ResultObj->SetObjectField(TEXT("actor_details"), FCommonUtils::ActorToJsonObject(TargetActor, true));
		return ResultObj;
	}
	else
	{
		return FCommonUtils::CreateErrorResponse(ErrorMessage);
	}
}

TSharedPtr<FJsonObject> FEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
	 // Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	// Find the blueprint
	if (BlueprintName.IsEmpty())
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Blueprint name is empty"));
	}

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Ensure path ends with /
	if (!BlueprintPath.EndsWith(TEXT("/")))
	{
		BlueprintPath += TEXT("/");
	}

	FString AssetPath = BlueprintPath + BlueprintName;

	if (!FPackageName::DoesPackageExist(AssetPath))
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found in path %s"), *BlueprintName, *BlueprintPath));
	}

	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Get transform parameters
	FVector Location(0.0f, 0.0f, 0.0f);
	FRotator Rotation(0.0f, 0.0f, 0.0f);
	FVector Scale(1.0f, 1.0f, 1.0f);

	if (Params->HasField(TEXT("location")))
	{
		Location = FCommonUtils::GetVectorFromJson(Params, TEXT("location"));
	}
	if (Params->HasField(TEXT("rotation")))
	{
		Rotation = FCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
	}
	if (Params->HasField(TEXT("scale")))
	{
		Scale = FCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
	}

	// Spawn the actor
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
	}

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(Location);
	SpawnTransform.SetRotation(FQuat(Rotation));
	SpawnTransform.SetScale3D(Scale);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = *ActorName;

	AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParams);
	if (NewActor)
	{
		// Set the Actor Label (display name in Outliner) to match the requested name
		NewActor->SetActorLabel(ActorName);

		UE_LOG(LogTemp, Display, TEXT("FEditorCommands: Spawned blueprint actor '%s' (label: '%s') at %s"),
		       *NewActor->GetName(), *ActorName, *Location.ToString());

		return FCommonUtils::ActorToJsonObject(NewActor, true);
	}

	return FCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FEditorCommands::HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
	}

	// Get color parameter [R, G, B] (values 0.0-1.0)
	TArray<float> ColorArray;
	const TArray<TSharedPtr<FJsonValue>>* ColorJsonArray;
	if (!Params->TryGetArrayField(TEXT("color"), ColorJsonArray) || ColorJsonArray->Num() < 3)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("'color' must be an array of 3 float values [R, G, B]"));
	}

	for (int32 i = 0; i < 3; i++)
	{
		ColorArray.Add(FMath::Clamp((float)(*ColorJsonArray)[i]->AsNumber(), 0.0f, 1.0f));
	}

	FLinearColor Color(ColorArray[0], ColorArray[1], ColorArray[2], 1.0f);

	// Get optional path parameter (default: /Game/Materials/)
	FString MaterialPath = TEXT("/Game/Materials/");
	Params->TryGetStringField(TEXT("material_path"), MaterialPath);

	// Ensure path ends with /
	if (!MaterialPath.EndsWith(TEXT("/")))
	{
		MaterialPath += TEXT("/");
	}

	// Create the material package
	FString FullPath = MaterialPath + MaterialName;

	// Check if material already exists
	if (UEditorAssetLibrary::DoesAssetExist(FullPath))
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material already exists: %s"), *FullPath));
	}

	// Create the package
	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create package for material"));
	}

	// Create the material using factory
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
		UMaterial::StaticClass(),
		Package,
		*MaterialName,
		RF_Standalone | RF_Public,
		nullptr,
		GWarn
	));

	if (!NewMaterial)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create material"));
	}

	// Set up the material with base color
	// Create a constant color expression
	UMaterialExpressionConstant3Vector* ColorExpression = NewObject<UMaterialExpressionConstant3Vector>(NewMaterial);
	if (ColorExpression)
	{
		ColorExpression->Constant = Color;
		NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(ColorExpression);

		// Connect to base color
		NewMaterial->GetEditorOnlyData()->BaseColor.Expression = ColorExpression;
	}

	// Compile the material
	NewMaterial->PreEditChange(nullptr);
	NewMaterial->PostEditChange();

	// Notify asset registry
	FAssetRegistryModule::AssetCreated(NewMaterial);

	// Mark package dirty
	Package->MarkPackageDirty();

	// Build result
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("name"), MaterialName);
	ResultObj->SetStringField(TEXT("path"), FullPath);

	TArray<TSharedPtr<FJsonValue>> ColorResultArray;
	ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.R));
	ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.G));
	ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.B));
	ResultObj->SetArrayField(TEXT("color"), ColorResultArray);

	ResultObj->SetBoolField(TEXT("success"), true);

	UE_LOG(LogTemp, Display, TEXT("FEditorCommands::HandleCreateMaterial: Created material '%s' with color R=%f, G=%f, B=%f"),
		*MaterialName, Color.R, Color.G, Color.B);

	return ResultObj;
}

// ============================================================================
// Actor Commands
// ============================================================================

TSharedPtr<FJsonObject> FEditorCommands::HandleSearchActors(const TSharedPtr<FJsonObject>& Params)
{
	// Get search pattern
	FString Pattern;
	Params->TryGetStringField(TEXT("pattern"), Pattern);

	// Get optional class filter
	FString ClassFilter;
	Params->TryGetStringField(TEXT("class_filter"), ClassFilter);

	// Get limit
	int32 Limit = 100;
	if (Params->HasField(TEXT("limit")))
	{
		Limit = Params->GetIntegerField(TEXT("limit"));
	}

	// Level Instance options
	bool bIncludeLevelInstances = true;
	Params->TryGetBoolField(TEXT("include_level_instances"), bIncludeLevelInstances);

	FString LevelInstanceFilter;
	Params->TryGetStringField(TEXT("level_instance_filter"), LevelInstanceFilter);

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
	}

	UWorldPartition* WorldPartition = GetWorldPartition();
	TArray<TSharedPtr<FJsonValue>> ResultsArray;
	int32 TotalFound = 0;
	int32 LoadedCount = 0;
	int32 UnloadedCount = 0;
	int32 LevelInstanceActorCount = 0;

	if (WorldPartition)
	{
		// World Partition enabled - iterate through ActorDescInstances (UE 5.6+)
		FWorldPartitionHelpers::ForEachActorDescInstance(WorldPartition, AActor::StaticClass(), [&](const FWorldPartitionActorDescInstance* ActorDescInstance)
		{
			if (!ActorDescInstance)
			{
				return true; // continue
			}

			// Check class filter
			if (!ClassFilter.IsEmpty())
			{
				UClass* ActorClass = ActorDescInstance->GetActorNativeClass();
				if (!ActorClass || !ActorClass->GetName().Contains(ClassFilter))
				{
					return true; // continue
				}
			}

			// Check name pattern (also check Actor Label)
			FString ActorName = ActorDescInstance->GetActorName().ToString();
			FString ActorLabel = ActorDescInstance->GetActorLabel().ToString();
			bool bNameMatch = Pattern.IsEmpty() ||
				ActorName.Contains(Pattern, ESearchCase::IgnoreCase) ||
				ActorLabel.Contains(Pattern, ESearchCase::IgnoreCase);

			if (!bNameMatch)
			{
				return true; // continue
			}

			TotalFound++;

			// Check if actor is loaded
			bool bIsLoaded = ActorDescInstance->GetActor() != nullptr;

			if (bIsLoaded)
			{
				LoadedCount++;
			}
			else
			{
				UnloadedCount++;
			}

			// Apply limit
			if (ResultsArray.Num() >= Limit)
			{
				return true; // continue but don't add more
			}

			ResultsArray.Add(MakeShared<FJsonValueObject>(ActorDescInstanceToJson(ActorDescInstance, bIsLoaded)));
			return true;
		});
	}
	else
	{
		// Non-WP map - use ForEachActorInWorld for Level Instance support
		FCommonUtils::ForEachActorInWorld(World, [&](AActor* Actor, ALevelInstance* OwningLI) -> bool
		{
			if (!Actor)
			{
				return true;
			}

			// Apply Level Instance filter
			if (!LevelInstanceFilter.IsEmpty() && OwningLI)
			{
				if (!OwningLI->GetName().Contains(LevelInstanceFilter, ESearchCase::IgnoreCase) &&
					!OwningLI->GetActorLabel().Contains(LevelInstanceFilter, ESearchCase::IgnoreCase))
				{
					return true;
				}
			}

			// Check class filter
			if (!ClassFilter.IsEmpty() && !Actor->GetClass()->GetName().Contains(ClassFilter))
			{
				return true;
			}

			// Check name pattern (also check Actor Label)
			bool bNameMatch = Pattern.IsEmpty() ||
				Actor->GetName().Contains(Pattern, ESearchCase::IgnoreCase) ||
				Actor->GetActorLabel().Contains(Pattern, ESearchCase::IgnoreCase);

			if (!bNameMatch)
			{
				return true;
			}

			TotalFound++;
			LoadedCount++;
			if (OwningLI)
			{
				LevelInstanceActorCount++;
			}

			if (ResultsArray.Num() >= Limit)
			{
				return true;
			}

			// Use standard actor info for non-WP maps
			TSharedPtr<FJsonObject> ActorInfo = MakeShared<FJsonObject>();
			ActorInfo->SetStringField(TEXT("name"), Actor->GetName());
			ActorInfo->SetStringField(TEXT("label"), Actor->GetActorLabel());
			ActorInfo->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
			ActorInfo->SetBoolField(TEXT("is_loaded"), true);

			FVector Location = Actor->GetActorLocation();
			TArray<TSharedPtr<FJsonValue>> LocationArray;
			LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
			LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
			LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
			ActorInfo->SetArrayField(TEXT("location"), LocationArray);

			if (OwningLI)
			{
				ActorInfo->SetStringField(TEXT("level_instance"), OwningLI->GetName());
				ActorInfo->SetStringField(TEXT("level_instance_label"), OwningLI->GetActorLabel());
			}

			ResultsArray.Add(MakeShared<FJsonValueObject>(ActorInfo));
			return true;
		}, bIncludeLevelInstances);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetBoolField(TEXT("is_world_partition"), WorldPartition != nullptr);
	ResultObj->SetNumberField(TEXT("result_count"), ResultsArray.Num());
	ResultObj->SetNumberField(TEXT("total_found"), TotalFound);
	ResultObj->SetNumberField(TEXT("loaded_count"), LoadedCount);
	ResultObj->SetNumberField(TEXT("unloaded_count"), UnloadedCount);
	ResultObj->SetNumberField(TEXT("level_instance_actor_count"), LevelInstanceActorCount);
	ResultObj->SetArrayField(TEXT("actors"), ResultsArray);
	return ResultObj;
}

// ============================================================================
// Material Commands
// ============================================================================

TSharedPtr<FJsonObject> FEditorCommands::HandleApplyMaterialToActor(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
	}

	// Get optional material slot (default 0)
	int32 MaterialSlot = 0;
	if (Params->HasField(TEXT("material_slot")))
	{
		MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
	}

	// Find the actor with auto-load support for World Partition
	bool bWasAutoLoaded = false;
	AActor* TargetActor = FCommonUtils::FindActorByNameWithAutoLoad(GWorld, ActorName, bWasAutoLoaded);
	if (!TargetActor)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s (searched both loaded actors and World Partition)"), *ActorName));
	}

	// Get the mesh component (try StaticMeshComponent first, then any PrimitiveComponent)
	UPrimitiveComponent* MeshComponent = nullptr;

	// Try to get StaticMeshComponent
	UStaticMeshComponent* StaticMeshComp = TargetActor->FindComponentByClass<UStaticMeshComponent>();
	if (StaticMeshComp)
	{
		MeshComponent = StaticMeshComp;
	}
	else
	{
		// Fall back to any PrimitiveComponent
		MeshComponent = TargetActor->FindComponentByClass<UPrimitiveComponent>();
	}

	if (!MeshComponent)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor '%s' has no mesh component"), *ActorName));
	}

	// Load the material asset
	UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
	if (!Material)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
	}

	// Apply the material
	MeshComponent->SetMaterial(MaterialSlot, Material);

	UE_LOG(LogTemp, Display, TEXT("FEditorCommands::HandleApplyMaterialToActor: Applied material '%s' to actor '%s' slot %d%s"),
		*MaterialPath, *ActorName, MaterialSlot,
		bWasAutoLoaded ? TEXT(" (auto-loaded from World Partition)") : TEXT(""));

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("actor"), ActorName);
	ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
	ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
	ResultObj->SetBoolField(TEXT("success"), true);
	if (bWasAutoLoaded)
	{
		ResultObj->SetBoolField(TEXT("was_auto_loaded_from_world_partition"), true);
	}
	return ResultObj;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleGetActorMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	// Find the actor with auto-load support for World Partition
	bool bWasAutoLoaded = false;
	AActor* TargetActor = FCommonUtils::FindActorByNameWithAutoLoad(GWorld, ActorName, bWasAutoLoaded);
	if (!TargetActor)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s (searched both loaded actors and World Partition)"), *ActorName));
	}

	// Get all primitive components and their materials
	TArray<TSharedPtr<FJsonValue>> ComponentsArray;

	TArray<UPrimitiveComponent*> PrimComponents;
	TargetActor->GetComponents<UPrimitiveComponent>(PrimComponents);

	for (UPrimitiveComponent* PrimComp : PrimComponents)
	{
		if (!PrimComp) continue;

		TSharedPtr<FJsonObject> CompInfo = MakeShared<FJsonObject>();
		CompInfo->SetStringField(TEXT("component_name"), PrimComp->GetName());
		CompInfo->SetStringField(TEXT("component_class"), PrimComp->GetClass()->GetName());

		// Get materials for this component
		TArray<TSharedPtr<FJsonValue>> MaterialsArray;
		int32 NumMaterials = PrimComp->GetNumMaterials();

		for (int32 i = 0; i < NumMaterials; i++)
		{
			TSharedPtr<FJsonObject> MaterialInfo = MakeShared<FJsonObject>();
			MaterialInfo->SetNumberField(TEXT("slot"), i);

			UMaterialInterface* Material = PrimComp->GetMaterial(i);
			if (Material)
			{
				MaterialInfo->SetStringField(TEXT("name"), Material->GetName());
				MaterialInfo->SetStringField(TEXT("path"), Material->GetPathName());

				// Check if it's a dynamic material instance
				UMaterialInstanceDynamic* DynMaterial = Cast<UMaterialInstanceDynamic>(Material);
				MaterialInfo->SetBoolField(TEXT("is_dynamic"), DynMaterial != nullptr);

				// Get parent material if it's a material instance
				UMaterialInstance* MatInstance = Cast<UMaterialInstance>(Material);
				if (MatInstance && MatInstance->Parent)
				{
					MaterialInfo->SetStringField(TEXT("parent_material"), MatInstance->Parent->GetPathName());
				}
			}
			else
			{
				MaterialInfo->SetStringField(TEXT("name"), TEXT("None"));
				MaterialInfo->SetStringField(TEXT("path"), TEXT(""));
				MaterialInfo->SetBoolField(TEXT("is_dynamic"), false);
			}

			MaterialsArray.Add(MakeShared<FJsonValueObject>(MaterialInfo));
		}

		CompInfo->SetNumberField(TEXT("material_count"), NumMaterials);
		CompInfo->SetArrayField(TEXT("materials"), MaterialsArray);

		ComponentsArray.Add(MakeShared<FJsonValueObject>(CompInfo));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("actor"), ActorName);
	ResultObj->SetStringField(TEXT("actor_class"), TargetActor->GetClass()->GetName());
	ResultObj->SetNumberField(TEXT("component_count"), ComponentsArray.Num());
	ResultObj->SetArrayField(TEXT("components"), ComponentsArray);
	ResultObj->SetBoolField(TEXT("success"), true);
	if (bWasAutoLoaded)
	{
		ResultObj->SetBoolField(TEXT("was_auto_loaded_from_world_partition"), true);
	}
	return ResultObj;
}

// ============================================================================
// Asset Search Commands
// ============================================================================

void FEditorCommands::AddAssetTypeFilter(FARFilter& Filter, const FString& AssetType)
{
	if (AssetType.IsEmpty())
	{
		return; // No filter - search all types
	}

	FString TypeUpper = AssetType.ToUpper();

	if (TypeUpper == TEXT("BLUEPRINT"))
	{
		Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	}
	else if (TypeUpper == TEXT("MATERIAL"))
	{
		Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
		Filter.ClassPaths.Add(UMaterialInstance::StaticClass()->GetClassPathName());
		Filter.ClassPaths.Add(UMaterialInstanceConstant::StaticClass()->GetClassPathName());
	}
	else if (TypeUpper == TEXT("STATICMESH"))
	{
		Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	}
	else if (TypeUpper == TEXT("SKELETALMESH"))
	{
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SkeletalMesh")));
	}
	else if (TypeUpper == TEXT("TEXTURE"))
	{
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Texture")));
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Texture2D")));
	}
	else if (TypeUpper == TEXT("SOUND"))
	{
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SoundWave")));
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SoundCue")));
	}
	else if (TypeUpper == TEXT("ANIMATION"))
	{
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("AnimSequence")));
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("AnimMontage")));
	}
	else if (TypeUpper == TEXT("NIAGARA"))
	{
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Niagara"), TEXT("NiagaraSystem")));
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Niagara"), TEXT("NiagaraEmitter")));
	}
	else if (TypeUpper == TEXT("WIDGET"))
	{
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/UMGEditor"), TEXT("WidgetBlueprint")));
	}
	else if (TypeUpper == TEXT("DATATABLE"))
	{
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("DataTable")));
	}
	else if (TypeUpper == TEXT("DATAASSET"))
	{
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("DataAsset")));
	}
	else if (TypeUpper == TEXT("WORLD"))
	{
		Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
	}
	// If unknown type, don't add any filter (search all types)
}

TArray<TSharedPtr<FJsonValue>> FEditorCommands::SearchClasses(const FString& SearchName, const FString& BaseClass, int32 Limit)
{
	TArray<TSharedPtr<FJsonValue>> Results;
	FString SearchNameLower = SearchName.ToLower();

	// Determine base class for filtering
	UClass* BaseClassFilter = nullptr;
	if (!BaseClass.IsEmpty())
	{
		FString ClassName = BaseClass;
		if (!ClassName.StartsWith(TEXT("U")) && !ClassName.StartsWith(TEXT("A")))
		{
			ClassName = TEXT("U") + ClassName;
		}
		BaseClassFilter = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::ExactClass);
	}

	// Iterate all UClass objects
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class) continue;

		// Skip deprecated and abstract classes
		if (Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists)) continue;

		// Apply base class filter
		if (BaseClassFilter && !Class->IsChildOf(BaseClassFilter)) continue;

		FString ClassName = Class->GetName().ToLower();
		if (ClassName.Contains(SearchNameLower))
		{
			TSharedPtr<FJsonObject> ClassInfo = MakeShared<FJsonObject>();
			ClassInfo->SetStringField(TEXT("name"), Class->GetName());
			ClassInfo->SetStringField(TEXT("path"), Class->GetPathName());
			ClassInfo->SetStringField(TEXT("type"), TEXT("Class"));

			if (UClass* ParentClass = Class->GetSuperClass())
			{
				ClassInfo->SetStringField(TEXT("parent"), ParentClass->GetName());
			}

			Results.Add(MakeShared<FJsonValueObject>(ClassInfo));

			if (Results.Num() >= Limit) break;
		}
	}

	return Results;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleSearchAssets(const TSharedPtr<FJsonObject>& Params)
{
	FString SearchName;
	if (!Params->TryGetStringField(TEXT("name"), SearchName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString SearchScope;
	if (!Params->TryGetStringField(TEXT("search_scope"), SearchScope))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'search_scope' parameter"));
	}

	FString ObjectType;
	Params->TryGetStringField(TEXT("object_type"), ObjectType);

	// Default to root path "/" to search entire project including plugins
	FString SearchPath = TEXT("/");
	Params->TryGetStringField(TEXT("search_path"), SearchPath);

	// For class search, base_class filters inheritance
	FString BaseClass;
	Params->TryGetStringField(TEXT("base_class"), BaseClass);

	int32 Limit = 50;
	if (Params->HasField(TEXT("limit")))
	{
		Limit = Params->GetIntegerField(TEXT("limit"));
	}

	TArray<TSharedPtr<FJsonValue>> AssetsArray;
	TArray<TSharedPtr<FJsonValue>> ClassesArray;

	// Search assets
	if (SearchScope == TEXT("asset") || SearchScope == TEXT("all"))
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		FARFilter Filter;
		Filter.bRecursiveClasses = true;
		Filter.bRecursivePaths = true;

		if (SearchPath == TEXT("/"))
		{
			// Search all content paths
			Filter.PackagePaths.Add(FName(TEXT("/Game")));
			Filter.PackagePaths.Add(FName(TEXT("/Engine")));
			for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetEnabledPlugins())
			{
				if (Plugin->CanContainContent())
				{
					Filter.PackagePaths.Add(FName(*FString::Printf(TEXT("/%s"), *Plugin->GetName())));
				}
			}
		}
		else
		{
			Filter.PackagePaths.Add(FName(*SearchPath));
		}
		AddAssetTypeFilter(Filter, ObjectType);

		TArray<FAssetData> AllAssets;
		AssetRegistry.GetAssets(Filter, AllAssets);

		// Find base class for filtering if specified
		UClass* FilterBaseClass = nullptr;
		if (!BaseClass.IsEmpty())
		{
			FilterBaseClass = FCommonUtils::FindClassByName(BaseClass);
		}

		FString SearchNameLower = SearchName.ToLower();
		bool bMatchAll = (SearchName == TEXT("*") || SearchName.IsEmpty());
		for (const FAssetData& AssetData : AllAssets)
		{
			FString AssetName = AssetData.AssetName.ToString().ToLower();
			if (bMatchAll || AssetName.Contains(SearchNameLower))
			{
				// Filter by base class if specified (for Blueprint assets)
				if (FilterBaseClass)
				{
					FString AssetClassName = AssetData.AssetClassPath.GetAssetName().ToString();
					if (AssetClassName == TEXT("Blueprint") || AssetClassName.Contains(TEXT("Blueprint")))
					{
						FString ParentClassPath;
						if (AssetData.GetTagValue(TEXT("ParentClass"), ParentClassPath) ||
						    AssetData.GetTagValue(TEXT("NativeParentClass"), ParentClassPath))
						{
							FString ParentClassName;
							int32 DotIndex;
							if (ParentClassPath.FindLastChar('.', DotIndex))
							{
								ParentClassName = ParentClassPath.Mid(DotIndex + 1);
								ParentClassName.RemoveFromEnd(TEXT("'"));
							}
							UClass* ParentClass = FCommonUtils::FindClassByName(ParentClassName);
							if (!ParentClass || !ParentClass->IsChildOf(FilterBaseClass))
							{
								continue;
							}
						}
						else
						{
							continue;
						}
					}
					else
					{
						continue;
					}
				}

				TSharedPtr<FJsonObject> AssetInfo = MakeShared<FJsonObject>();
				AssetInfo->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
				AssetInfo->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
				AssetInfo->SetStringField(TEXT("type"), AssetData.AssetClassPath.GetAssetName().ToString());

				AssetsArray.Add(MakeShared<FJsonValueObject>(AssetInfo));

				if (AssetsArray.Num() >= Limit) break;
			}
		}
	}

	// Search classes
	if (SearchScope == TEXT("class") || SearchScope == TEXT("all"))
	{
		ClassesArray = SearchClasses(SearchName, BaseClass, Limit);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("search_name"), SearchName);
	ResultObj->SetStringField(TEXT("search_scope"), SearchScope);
	ResultObj->SetNumberField(TEXT("asset_count"), AssetsArray.Num());
	ResultObj->SetNumberField(TEXT("class_count"), ClassesArray.Num());
	ResultObj->SetArrayField(TEXT("assets"), AssetsArray);
	ResultObj->SetArrayField(TEXT("classes"), ClassesArray);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleListFolderAssets(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameter
	FString FolderPath;
	if (!Params->TryGetStringField(TEXT("folder_path"), FolderPath))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'folder_path' parameter"));
	}

	// Get optional parameters
	FString AssetType;
	Params->TryGetStringField(TEXT("asset_type"), AssetType);

	bool bRecursive = false;
	if (Params->HasField(TEXT("recursive")))
	{
		bRecursive = Params->GetBoolField(TEXT("recursive"));
	}

	int32 Limit = 100;
	if (Params->HasField(TEXT("limit")))
	{
		Limit = Params->GetIntegerField(TEXT("limit"));
	}

	// Get asset registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Build filter
	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = bRecursive;
	Filter.PackagePaths.Add(FName(*FolderPath));

	// Add type filter if specified
	AddAssetTypeFilter(Filter, AssetType);

	// Get assets
	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	// Build result array (with limit)
	TArray<TSharedPtr<FJsonValue>> AssetsArray;
	int32 Count = 0;
	for (const FAssetData& AssetData : AssetDataList)
	{
		if (Count >= Limit)
		{
			break;
		}

		TSharedPtr<FJsonObject> AssetInfo = MakeShared<FJsonObject>();
		AssetInfo->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		AssetInfo->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
		AssetInfo->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());

		AssetsArray.Add(MakeShared<FJsonValueObject>(AssetInfo));
		Count++;
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("folder_path"), FolderPath);
	ResultObj->SetStringField(TEXT("asset_type"), AssetType.IsEmpty() ? TEXT("All") : AssetType);
	ResultObj->SetBoolField(TEXT("recursive"), bRecursive);
	ResultObj->SetNumberField(TEXT("asset_count"), AssetsArray.Num());
	ResultObj->SetNumberField(TEXT("total_found"), AssetDataList.Num());
	ResultObj->SetArrayField(TEXT("assets"), AssetsArray);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

// ============================================================================
// World Partition Commands
// ============================================================================

UWorldPartition* FEditorCommands::GetWorldPartition()
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return nullptr;
	}
	return World->GetWorldPartition();
}

TSharedPtr<FJsonObject> FEditorCommands::ActorDescInstanceToJson(const FWorldPartitionActorDescInstance* ActorDescInstance, bool bIsLoaded)
{
	TSharedPtr<FJsonObject> ActorInfo = MakeShared<FJsonObject>();

	if (!ActorDescInstance)
	{
		return ActorInfo;
	}

	// Basic info
	ActorInfo->SetStringField(TEXT("guid"), ActorDescInstance->GetGuid().ToString());
	ActorInfo->SetStringField(TEXT("name"), ActorDescInstance->GetActorName().ToString());
	ActorInfo->SetStringField(TEXT("class"), ActorDescInstance->GetActorNativeClass() ? ActorDescInstance->GetActorNativeClass()->GetName() : TEXT("Unknown"));
	ActorInfo->SetStringField(TEXT("label"), ActorDescInstance->GetActorLabel().ToString());
	ActorInfo->SetBoolField(TEXT("is_loaded"), bIsLoaded);

	// Bounds info
	FBox Bounds = ActorDescInstance->GetEditorBounds();
	if (Bounds.IsValid)
	{
		TSharedPtr<FJsonObject> BoundsObj = MakeShared<FJsonObject>();

		TArray<TSharedPtr<FJsonValue>> MinArray;
		MinArray.Add(MakeShared<FJsonValueNumber>(Bounds.Min.X));
		MinArray.Add(MakeShared<FJsonValueNumber>(Bounds.Min.Y));
		MinArray.Add(MakeShared<FJsonValueNumber>(Bounds.Min.Z));
		BoundsObj->SetArrayField(TEXT("min"), MinArray);

		TArray<TSharedPtr<FJsonValue>> MaxArray;
		MaxArray.Add(MakeShared<FJsonValueNumber>(Bounds.Max.X));
		MaxArray.Add(MakeShared<FJsonValueNumber>(Bounds.Max.Y));
		MaxArray.Add(MakeShared<FJsonValueNumber>(Bounds.Max.Z));
		BoundsObj->SetArrayField(TEXT("max"), MaxArray);

		FVector Center = Bounds.GetCenter();
		TArray<TSharedPtr<FJsonValue>> CenterArray;
		CenterArray.Add(MakeShared<FJsonValueNumber>(Center.X));
		CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Y));
		CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Z));
		BoundsObj->SetArrayField(TEXT("center"), CenterArray);

		ActorInfo->SetObjectField(TEXT("bounds"), BoundsObj);
	}

	// Package path (for OFPA file reference)
	ActorInfo->SetStringField(TEXT("actor_package"), ActorDescInstance->GetActorPackage().ToString());

	return ActorInfo;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleSearchActorsInRegion(const TSharedPtr<FJsonObject>& Params)
{
	// Get region center
	FVector Center(0, 0, 0);
	if (Params->HasField(TEXT("center")))
	{
		Center = FCommonUtils::GetVectorFromJson(Params, TEXT("center"));
	}
	else if (Params->HasField(TEXT("x")) && Params->HasField(TEXT("y")))
	{
		Center.X = Params->GetNumberField(TEXT("x"));
		Center.Y = Params->GetNumberField(TEXT("y"));
		if (Params->HasField(TEXT("z")))
		{
			Center.Z = Params->GetNumberField(TEXT("z"));
		}
	}

	// Get radius or box extent
	float Radius = 10000.0f; // Default 100m
	if (Params->HasField(TEXT("radius")))
	{
		Radius = Params->GetNumberField(TEXT("radius"));
	}

	// Optional box extent (overrides radius if provided)
	FVector Extent(Radius, Radius, Radius);
	if (Params->HasField(TEXT("extent")))
	{
		Extent = FCommonUtils::GetVectorFromJson(Params, TEXT("extent"));
	}

	// Get optional class filter
	FString ClassFilter;
	Params->TryGetStringField(TEXT("class_filter"), ClassFilter);

	// Get limit
	int32 Limit = 100;
	if (Params->HasField(TEXT("limit")))
	{
		Limit = Params->GetIntegerField(TEXT("limit"));
	}

	// Create search box
	FBox SearchBox(Center - Extent, Center + Extent);

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
	}

	UWorldPartition* WorldPartition = GetWorldPartition();
	TArray<TSharedPtr<FJsonValue>> ResultsArray;
	int32 TotalFound = 0;

	if (WorldPartition)
	{
		// World Partition - search through ActorDescInstances (UE 5.6+)
		FWorldPartitionHelpers::ForEachActorDescInstance(WorldPartition, AActor::StaticClass(), [&](const FWorldPartitionActorDescInstance* ActorDescInstance) -> bool
		{
			if (!ActorDescInstance)
			{
				return true; // continue
			}

			// Check class filter
			if (!ClassFilter.IsEmpty())
			{
				UClass* ActorClass = ActorDescInstance->GetActorNativeClass();
				if (!ActorClass || !ActorClass->GetName().Contains(ClassFilter))
				{
					return true; // continue
				}
			}

			// Check if actor bounds intersect with search box
			FBox ActorBounds = ActorDescInstance->GetEditorBounds();
			if (!ActorBounds.IsValid || !SearchBox.Intersect(ActorBounds))
			{
				return true; // continue
			}

			TotalFound++;

			if (ResultsArray.Num() < Limit)
			{
				bool bIsLoaded = ActorDescInstance->GetActor() != nullptr;
				ResultsArray.Add(MakeShared<FJsonValueObject>(ActorDescInstanceToJson(ActorDescInstance, bIsLoaded)));
			}

			return true; // continue
		});
	}
	else
	{
		// Non-WP map
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor)
			{
				continue;
			}

			// Check class filter
			if (!ClassFilter.IsEmpty() && !Actor->GetClass()->GetName().Contains(ClassFilter))
			{
				continue;
			}

			// Check if actor is within search box
			FVector ActorLocation = Actor->GetActorLocation();
			if (!SearchBox.IsInside(ActorLocation))
			{
				continue;
			}

			TotalFound++;

			if (ResultsArray.Num() < Limit)
			{
				TSharedPtr<FJsonObject> ActorInfo = MakeShared<FJsonObject>();
				ActorInfo->SetStringField(TEXT("name"), Actor->GetName());
				ActorInfo->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
				ActorInfo->SetBoolField(TEXT("is_loaded"), true);

				TArray<TSharedPtr<FJsonValue>> LocationArray;
				LocationArray.Add(MakeShared<FJsonValueNumber>(ActorLocation.X));
				LocationArray.Add(MakeShared<FJsonValueNumber>(ActorLocation.Y));
				LocationArray.Add(MakeShared<FJsonValueNumber>(ActorLocation.Z));
				ActorInfo->SetArrayField(TEXT("location"), LocationArray);

				ResultsArray.Add(MakeShared<FJsonValueObject>(ActorInfo));
			}
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetBoolField(TEXT("is_world_partition"), WorldPartition != nullptr);

	// Include search region info
	TArray<TSharedPtr<FJsonValue>> CenterArray;
	CenterArray.Add(MakeShared<FJsonValueNumber>(Center.X));
	CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Y));
	CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Z));
	ResultObj->SetArrayField(TEXT("search_center"), CenterArray);
	ResultObj->SetNumberField(TEXT("search_radius"), Radius);

	ResultObj->SetNumberField(TEXT("result_count"), ResultsArray.Num());
	ResultObj->SetNumberField(TEXT("total_found"), TotalFound);
	ResultObj->SetArrayField(TEXT("actors"), ResultsArray);
	return ResultObj;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleLoadActorByGuid(const TSharedPtr<FJsonObject>& Params)
{
	FString GuidString;
	if (!Params->TryGetStringField(TEXT("guid"), GuidString))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'guid' parameter"));
	}

	FGuid ActorGuid;
	if (!FGuid::Parse(GuidString, ActorGuid))
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid GUID format: %s"), *GuidString));
	}

	UWorldPartition* WorldPartition = GetWorldPartition();
	if (!WorldPartition)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("World Partition is not enabled for this map"));
	}

	// Find the actor desc by GUID (UE 5.6+)
	const FWorldPartitionActorDescInstance* FoundDesc = nullptr;
	FWorldPartitionHelpers::ForEachActorDescInstance(WorldPartition, AActor::StaticClass(), [&](const FWorldPartitionActorDescInstance* ActorDescInstance) -> bool
	{
		if (ActorDescInstance && ActorDescInstance->GetGuid() == ActorGuid)
		{
			FoundDesc = ActorDescInstance;
			return false; // stop
		}
		return true; // continue
	});

	if (!FoundDesc)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found with GUID: %s"), *GuidString));
	}

	// Check if already loaded
	AActor* ExistingActor = FoundDesc->GetActor();
	if (ExistingActor)
	{
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("success"), true);
		ResultObj->SetBoolField(TEXT("was_already_loaded"), true);
		ResultObj->SetStringField(TEXT("actor_name"), ExistingActor->GetName());
		ResultObj->SetStringField(TEXT("guid"), GuidString);
		return ResultObj;
	}

	// Load the actor using Reference mechanism
	// UE 5.6: FWorldPartitionReference takes WorldPartition and Guid directly
	FWorldPartitionReference ActorRef(WorldPartition, ActorGuid);

	// Try to get the loaded actor
	AActor* LoadedActor = FoundDesc->GetActor();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), LoadedActor != nullptr);
	ResultObj->SetBoolField(TEXT("was_already_loaded"), false);
	ResultObj->SetStringField(TEXT("guid"), GuidString);

	if (LoadedActor)
	{
		ResultObj->SetStringField(TEXT("actor_name"), LoadedActor->GetName());
		ResultObj->SetStringField(TEXT("actor_class"), LoadedActor->GetClass()->GetName());

		FVector Location = LoadedActor->GetActorLocation();
		TArray<TSharedPtr<FJsonValue>> LocationArray;
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
		ResultObj->SetArrayField(TEXT("location"), LocationArray);
	}
	else
	{
		ResultObj->SetStringField(TEXT("error"), TEXT("Actor handle created but actor not loaded. Try using set_region_loaded with loaded=true instead."));
	}

	return ResultObj;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleSetRegionLoaded(const TSharedPtr<FJsonObject>& Params)
{
	// Get loaded state
	bool bIsLoad = true;
	if (!Params->TryGetBoolField(TEXT("loaded"), bIsLoad))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'loaded' parameter (true/false)."));
	}

	// Get region center
	FVector Center(0, 0, 0);
	if (Params->HasField(TEXT("center")))
	{
		Center = FCommonUtils::GetVectorFromJson(Params, TEXT("center"));
	}
	else if (Params->HasField(TEXT("x")) && Params->HasField(TEXT("y")) && Params->HasField(TEXT("z")))
	{
		Center.X = Params->GetNumberField(TEXT("x"));
		Center.Y = Params->GetNumberField(TEXT("y"));
		Center.Z = Params->GetNumberField(TEXT("z"));
	}
	else
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing center coordinates"));
	}

	// Get radius
	if (!Params->HasField(TEXT("radius")))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'radius' parameter"));
	}
	float Radius = Params->GetNumberField(TEXT("radius"));

	UWorldPartition* WorldPartition = GetWorldPartition();
	if (!WorldPartition)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("World Partition is not enabled for this map"));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetBoolField(TEXT("loaded"), bIsLoad);

	TArray<TSharedPtr<FJsonValue>> CenterArray;
	CenterArray.Add(MakeShared<FJsonValueNumber>(Center.X));
	CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Y));
	CenterArray.Add(MakeShared<FJsonValueNumber>(Center.Z));
	ResultObj->SetArrayField(TEXT("center"), CenterArray);
	ResultObj->SetNumberField(TEXT("radius"), Radius);

	if (bIsLoad)
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!World)
		{
			return FCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
		}

		// Create a box for the region
		FVector Extent(Radius, Radius, Radius);
		FBox RegionBox(Center - Extent, Center + Extent);

		// Collect actors in the region and load them
		TArray<FGuid> ActorsToLoad;
		FWorldPartitionHelpers::ForEachActorDescInstance(WorldPartition, AActor::StaticClass(), [&](const FWorldPartitionActorDescInstance* ActorDescInstance) -> bool
		{
			if (!ActorDescInstance)
			{
				return true;
			}

			FBox ActorBounds = ActorDescInstance->GetEditorBounds();
			if (ActorBounds.IsValid && RegionBox.Intersect(ActorBounds))
			{
				if (!ActorDescInstance->GetActor())
				{
					ActorsToLoad.Add(ActorDescInstance->GetGuid());
				}
			}

			return true;
		});

		// Load actors by creating handles/references
		int32 LoadedCount = 0;
		TArray<FWorldPartitionReference> LoadedRefs;

		for (const FGuid& ActorGuid : ActorsToLoad)
		{
			FWorldPartitionReference Ref(WorldPartition, ActorGuid);
			if (Ref.IsValid())
			{
				LoadedRefs.Add(MoveTemp(Ref));
				LoadedCount++;
			}
		}

		ResultObj->SetNumberField(TEXT("actors_found"), ActorsToLoad.Num());
		ResultObj->SetNumberField(TEXT("actors_loaded"), LoadedCount);
		ResultObj->SetStringField(TEXT("note"), TEXT("Actors are now pinned. Use search_actors to see updated status."));
	}
	else
	{
		ResultObj->SetStringField(TEXT("note"), TEXT("Region unload requested. Actual unloading depends on editor streaming state and pin references."));
	}

	return ResultObj;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleGetWorldPartitionInfo(const TSharedPtr<FJsonObject>& Params)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("world_name"), World->GetName());

	UWorldPartition* WorldPartition = GetWorldPartition();
	ResultObj->SetBoolField(TEXT("is_world_partition"), WorldPartition != nullptr);

	if (WorldPartition)
	{
		// Count actors
		int32 TotalActors = 0;
		int32 LoadedActors = 0;
		int32 UnloadedActors = 0;
		FBox WorldBounds(ForceInit);

		// UE 5.6+: Use ForEachActorDescInstance
		FWorldPartitionHelpers::ForEachActorDescInstance(WorldPartition, AActor::StaticClass(), [&](const FWorldPartitionActorDescInstance* ActorDescInstance) -> bool
		{
			if (!ActorDescInstance)
			{
				return true; // continue
			}

			TotalActors++;

			if (ActorDescInstance->GetActor())
			{
				LoadedActors++;
			}
			else
			{
				UnloadedActors++;
			}

			// Expand world bounds
			FBox ActorBounds = ActorDescInstance->GetEditorBounds();
			if (ActorBounds.IsValid)
			{
				if (WorldBounds.IsValid)
				{
					WorldBounds = WorldBounds + ActorBounds;
				}
				else
				{
					WorldBounds = ActorBounds;
				}
			}

			return true; // continue
		});

		ResultObj->SetNumberField(TEXT("total_actors"), TotalActors);
		ResultObj->SetNumberField(TEXT("loaded_actors"), LoadedActors);
		ResultObj->SetNumberField(TEXT("unloaded_actors"), UnloadedActors);

		if (WorldBounds.IsValid)
		{
			TSharedPtr<FJsonObject> BoundsObj = MakeShared<FJsonObject>();

			TArray<TSharedPtr<FJsonValue>> MinArray;
			MinArray.Add(MakeShared<FJsonValueNumber>(WorldBounds.Min.X));
			MinArray.Add(MakeShared<FJsonValueNumber>(WorldBounds.Min.Y));
			MinArray.Add(MakeShared<FJsonValueNumber>(WorldBounds.Min.Z));
			BoundsObj->SetArrayField(TEXT("min"), MinArray);

			TArray<TSharedPtr<FJsonValue>> MaxArray;
			MaxArray.Add(MakeShared<FJsonValueNumber>(WorldBounds.Max.X));
			MaxArray.Add(MakeShared<FJsonValueNumber>(WorldBounds.Max.Y));
			MaxArray.Add(MakeShared<FJsonValueNumber>(WorldBounds.Max.Z));
			BoundsObj->SetArrayField(TEXT("max"), MaxArray);

			FVector Size = WorldBounds.GetSize();
			TArray<TSharedPtr<FJsonValue>> SizeArray;
			SizeArray.Add(MakeShared<FJsonValueNumber>(Size.X));
			SizeArray.Add(MakeShared<FJsonValueNumber>(Size.Y));
			SizeArray.Add(MakeShared<FJsonValueNumber>(Size.Z));
			BoundsObj->SetArrayField(TEXT("size"), SizeArray);

			ResultObj->SetObjectField(TEXT("world_bounds"), BoundsObj);
		}
	}
	else
	{
		// Non-WP map - count loaded actors only
		int32 ActorCount = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			ActorCount++;
		}
		ResultObj->SetNumberField(TEXT("total_actors"), ActorCount);
		ResultObj->SetNumberField(TEXT("loaded_actors"), ActorCount);
		ResultObj->SetNumberField(TEXT("unloaded_actors"), 0);
	}

	return ResultObj;
}

// ============================================================================
// GAS GameplayTag Commands
// ============================================================================

TSharedPtr<FJsonObject> FEditorCommands::HandleListGameplayTags(const TSharedPtr<FJsonObject>& Params)
{
	// Get filter parameters
	FString Prefix;
	Params->TryGetStringField(TEXT("prefix"), Prefix);

	int32 MaxDepth = 5;
	if (Params->HasField(TEXT("max_depth")))
	{
		MaxDepth = static_cast<int32>(Params->GetNumberField(TEXT("max_depth")));
	}

	int32 Limit = 100;
	if (Params->HasField(TEXT("limit")))
	{
		Limit = static_cast<int32>(Params->GetNumberField(TEXT("limit")));
	}

	// Get all gameplay tags from the manager
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();
	FGameplayTagContainer AllTags;
	TagsManager.RequestAllGameplayTags(AllTags, true);

	// Filter and collect tags
	TArray<FString> FilteredTags;
	int32 PrefixDepth = 0;

	// Calculate prefix depth (number of dots + 1)
	if (!Prefix.IsEmpty())
	{
		for (TCHAR Char : Prefix)
		{
			if (Char == TEXT('.'))
			{
				PrefixDepth++;
			}
		}
		// If prefix ends with dot, don't count it as extra depth
		if (!Prefix.EndsWith(TEXT(".")))
		{
			PrefixDepth++;
		}
	}

	for (const FGameplayTag& Tag : AllTags)
	{
		FString TagString = Tag.ToString();

		// Apply prefix filter
		if (!Prefix.IsEmpty())
		{
			if (!TagString.StartsWith(Prefix))
			{
				continue;
			}
		}

		// Apply depth filter
		int32 TagDepth = 1;
		for (TCHAR Char : TagString)
		{
			if (Char == TEXT('.'))
			{
				TagDepth++;
			}
		}

		// Check if within allowed depth from prefix
		int32 RelativeDepth = TagDepth - PrefixDepth;
		if (RelativeDepth > MaxDepth)
		{
			continue;
		}

		FilteredTags.Add(TagString);

		// Apply limit
		if (FilteredTags.Num() >= Limit)
		{
			break;
		}
	}

	// Sort alphabetically for consistent output
	FilteredTags.Sort();

	// Build result
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);

	TArray<TSharedPtr<FJsonValue>> TagsArray;
	for (const FString& TagStr : FilteredTags)
	{
		TagsArray.Add(MakeShared<FJsonValueString>(TagStr));
	}
	ResultObj->SetArrayField(TEXT("tags"), TagsArray);
	ResultObj->SetNumberField(TEXT("count"), FilteredTags.Num());
	ResultObj->SetNumberField(TEXT("total_in_project"), AllTags.Num());

	if (!Prefix.IsEmpty())
	{
		ResultObj->SetStringField(TEXT("prefix_filter"), Prefix);
	}
	ResultObj->SetNumberField(TEXT("max_depth"), MaxDepth);
	ResultObj->SetNumberField(TEXT("limit"), Limit);

	// Add hint if results were truncated
	if (FilteredTags.Num() >= Limit)
	{
		ResultObj->SetBoolField(TEXT("truncated"), true);
		ResultObj->SetStringField(TEXT("hint"), TEXT("Results truncated. Use prefix filter or increase limit for more results."));
	}

	return ResultObj;
}

// ============================================================================
// Level Instance Commands
// ============================================================================

TSharedPtr<FJsonObject> FEditorCommands::HandleListLevelInstances(const TSharedPtr<FJsonObject>& Params)
{
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
	}

	TArray<TSharedPtr<FJsonValue>> LevelInstancesArray;

	for (TActorIterator<ALevelInstance> It(World); It; ++It)
	{
		ALevelInstance* LI = *It;
		if (!LI)
		{
			continue;
		}

		TSharedPtr<FJsonObject> LIInfo = MakeShared<FJsonObject>();
		LIInfo->SetStringField(TEXT("name"), LI->GetName());
		LIInfo->SetStringField(TEXT("label"), LI->GetActorLabel());
		LIInfo->SetStringField(TEXT("class"), LI->GetClass()->GetName());

		if (TSoftObjectPtr<UWorld> WorldAsset = LI->GetWorldAsset())
		{
			LIInfo->SetStringField(TEXT("world_asset"), WorldAsset.ToString());
		}

		FVector Location = LI->GetActorLocation();
		TArray<TSharedPtr<FJsonValue>> LocationArray;
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
		LIInfo->SetArrayField(TEXT("location"), LocationArray);

		FRotator Rotation = LI->GetActorRotation();
		TArray<TSharedPtr<FJsonValue>> RotationArray;
		RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
		RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
		RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
		LIInfo->SetArrayField(TEXT("rotation"), RotationArray);

		FVector Scale = LI->GetActorScale3D();
		TArray<TSharedPtr<FJsonValue>> ScaleArray;
		ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
		ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
		ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
		LIInfo->SetArrayField(TEXT("scale"), ScaleArray);

		ULevel* LoadedLevel = FCommonUtils::GetLevelInstanceLoadedLevel(LI);
		LIInfo->SetBoolField(TEXT("is_loaded"), LoadedLevel != nullptr);

		if (LoadedLevel)
		{
			int32 ActorCount = 0;
			for (AActor* A : LoadedLevel->Actors)
			{
				if (A && !A->IsA<ALevelInstanceEditorInstanceActor>())
				{
					ActorCount++;
				}
			}
			LIInfo->SetNumberField(TEXT("actor_count"), ActorCount);
		}

		LevelInstancesArray.Add(MakeShared<FJsonValueObject>(LIInfo));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetNumberField(TEXT("count"), LevelInstancesArray.Num());
	ResultObj->SetArrayField(TEXT("level_instances"), LevelInstancesArray);
	return ResultObj;
}

TSharedPtr<FJsonObject> FEditorCommands::HandleGetLevelInstanceActors(const TSharedPtr<FJsonObject>& Params)
{
	FString LevelInstanceName;
	if (!Params->TryGetStringField(TEXT("level_instance_name"), LevelInstanceName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'level_instance_name' parameter"));
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
	}

	// Find the Level Instance
	ALevelInstance* TargetLI = nullptr;
	for (TActorIterator<ALevelInstance> It(World); It; ++It)
	{
		ALevelInstance* LI = *It;
		if (LI && (LI->GetName() == LevelInstanceName ||
		           LI->GetActorLabel() == LevelInstanceName ||
		           LI->GetActorLabel().Contains(LevelInstanceName, ESearchCase::IgnoreCase)))
		{
			TargetLI = LI;
			break;
		}
	}

	if (!TargetLI)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Level Instance '%s' not found"), *LevelInstanceName));
	}

	ULevel* LoadedLevel = FCommonUtils::GetLevelInstanceLoadedLevel(TargetLI);
	if (!LoadedLevel)
	{
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("success"), true);
		ResultObj->SetStringField(TEXT("level_instance"), TargetLI->GetName());
		ResultObj->SetBoolField(TEXT("is_loaded"), false);
		ResultObj->SetStringField(TEXT("note"), TEXT("Level Instance is not currently loaded"));
		return ResultObj;
	}

	TArray<TSharedPtr<FJsonValue>> ActorsArray;

	for (AActor* Actor : LoadedLevel->Actors)
	{
		if (!Actor || Actor->IsA<ALevelInstanceEditorInstanceActor>())
		{
			continue;
		}

		TSharedPtr<FJsonObject> ActorInfo = MakeShared<FJsonObject>();
		ActorInfo->SetStringField(TEXT("name"), Actor->GetName());
		ActorInfo->SetStringField(TEXT("label"), Actor->GetActorLabel());
		ActorInfo->SetStringField(TEXT("class"), Actor->GetClass()->GetName());

		FVector Location = Actor->GetActorLocation();
		TArray<TSharedPtr<FJsonValue>> LocationArray;
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
		LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
		ActorInfo->SetArrayField(TEXT("location"), LocationArray);

		ActorsArray.Add(MakeShared<FJsonValueObject>(ActorInfo));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("level_instance"), TargetLI->GetName());
	ResultObj->SetStringField(TEXT("level_instance_label"), TargetLI->GetActorLabel());
	ResultObj->SetBoolField(TEXT("is_loaded"), true);
	ResultObj->SetNumberField(TEXT("actor_count"), ActorsArray.Num());
	ResultObj->SetArrayField(TEXT("actors"), ActorsArray);
	return ResultObj;
}