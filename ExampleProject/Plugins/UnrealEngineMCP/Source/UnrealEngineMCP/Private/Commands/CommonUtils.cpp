#include "Commands/CommonUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "EngineUtils.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionHelpers.h"
#include "WorldPartition/WorldPartitionActorDescInstance.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceInterface.h"
#include "LevelInstance/LevelInstanceEditorInstanceActor.h"

// Static storage for World Partition actor references (to keep actors loaded)
static TArray<FWorldPartitionReference> GAutoLoadedActorRefs;

// JSON Utilities
TSharedPtr<FJsonObject> FCommonUtils::CreateErrorResponse(const FString& Message)
{
	TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
	ResponseObject->SetBoolField(TEXT("success"), false);
	ResponseObject->SetStringField(TEXT("error"), Message);
	return ResponseObject;
}

TSharedPtr<FJsonObject> FCommonUtils::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
	TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
	ResponseObject->SetBoolField(TEXT("success"), true);
	
	if (Data.IsValid())
	{
		ResponseObject->SetObjectField(TEXT("data"), Data);
	}
	
	return ResponseObject;
}

void FCommonUtils::GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray)
{
	OutArray.Reset();
	
	if (!JsonObject->HasField(FieldName))
	{
		return;
	}
	
	const TArray<TSharedPtr<FJsonValue>>* JsonArray;
	if (JsonObject->TryGetArrayField(FieldName, JsonArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
		{
			OutArray.Add((int32)Value->AsNumber());
		}
	}
}

void FCommonUtils::GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray)
{
	OutArray.Reset();
	
	if (!JsonObject->HasField(FieldName))
	{
		return;
	}
	
	const TArray<TSharedPtr<FJsonValue>>* JsonArray;
	if (JsonObject->TryGetArrayField(FieldName, JsonArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
		{
			OutArray.Add((float)Value->AsNumber());
		}
	}
}

FVector2D FCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
	FVector2D Result(0.0f, 0.0f);
	
	if (!JsonObject->HasField(FieldName))
	{
		return Result;
	}
	
	const TArray<TSharedPtr<FJsonValue>>* JsonArray;
	if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 2)
	{
		Result.X = (float)(*JsonArray)[0]->AsNumber();
		Result.Y = (float)(*JsonArray)[1]->AsNumber();
	}
	
	return Result;
}

FVector FCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
	FVector Result(0.0f, 0.0f, 0.0f);
	
	if (!JsonObject->HasField(FieldName))
	{
		return Result;
	}
	
	const TArray<TSharedPtr<FJsonValue>>* JsonArray;
	if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
	{
		Result.X = (float)(*JsonArray)[0]->AsNumber();
		Result.Y = (float)(*JsonArray)[1]->AsNumber();
		Result.Z = (float)(*JsonArray)[2]->AsNumber();
	}
	
	return Result;
}

FRotator FCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
	FRotator Result(0.0f, 0.0f, 0.0f);
	
	if (!JsonObject->HasField(FieldName))
	{
		return Result;
	}
	
	const TArray<TSharedPtr<FJsonValue>>* JsonArray;
	if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
	{
		Result.Pitch = (float)(*JsonArray)[0]->AsNumber();
		Result.Yaw = (float)(*JsonArray)[1]->AsNumber();
		Result.Roll = (float)(*JsonArray)[2]->AsNumber();
	}
	
	return Result;
}

// Blueprint Utilities
UBlueprint* FCommonUtils::FindBlueprint(const FString& BlueprintName, const FString& BlueprintPath)
{
	return FindBlueprintByName(BlueprintName, BlueprintPath);
}

UBlueprint* FCommonUtils::FindBlueprintByName(const FString& BlueprintName, const FString& BlueprintPath)
{
	// Ensure path ends with /
	FString NormalizedPath = BlueprintPath;
	if (!NormalizedPath.EndsWith(TEXT("/")))
	{
		NormalizedPath += TEXT("/");
	}

	FString AssetPath = NormalizedPath + BlueprintName;
	return LoadObject<UBlueprint>(nullptr, *AssetPath);
}

// Actor utilities
TSharedPtr<FJsonValue> FCommonUtils::ActorToJson(AActor* Actor)
{
	if (!Actor)
	{
		return MakeShared<FJsonValueNull>();
	}

	TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
	ActorObject->SetStringField(TEXT("name"), Actor->GetName());
	ActorObject->SetStringField(TEXT("label"), Actor->GetActorLabel());
	ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
	
	FVector Location = Actor->GetActorLocation();
	TArray<TSharedPtr<FJsonValue>> LocationArray;
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
	ActorObject->SetArrayField(TEXT("location"), LocationArray);
	
	FRotator Rotation = Actor->GetActorRotation();
	TArray<TSharedPtr<FJsonValue>> RotationArray;
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
	ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
	
	FVector Scale = Actor->GetActorScale3D();
	TArray<TSharedPtr<FJsonValue>> ScaleArray;
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
	ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
	
	return MakeShared<FJsonValueObject>(ActorObject);
}

TSharedPtr<FJsonObject> FCommonUtils::ActorToJsonObject(AActor* Actor, bool bDetailed)
{
	if (!Actor)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
	ActorObject->SetStringField(TEXT("name"), Actor->GetName());
	ActorObject->SetStringField(TEXT("label"), Actor->GetActorLabel());
	ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
	ActorObject->SetBoolField(TEXT("success"), true);
	
	FVector Location = Actor->GetActorLocation();
	TArray<TSharedPtr<FJsonValue>> LocationArray;
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
	ActorObject->SetArrayField(TEXT("location"), LocationArray);
	
	FRotator Rotation = Actor->GetActorRotation();
	TArray<TSharedPtr<FJsonValue>> RotationArray;
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
	RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
	ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
	
	FVector Scale = Actor->GetActorScale3D();
	TArray<TSharedPtr<FJsonValue>> ScaleArray;
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
	ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
	
	return ActorObject;
}

AActor* FCommonUtils::FindActorByName(UWorld* World, const FString& ActorName)
{
	if (!World || ActorName.IsEmpty())
	{
		return nullptr;
	}

	// Try O(1) lookup by Object Name
	ULevel* CurrentLevel = World->GetCurrentLevel();
	if (CurrentLevel)
	{
		AActor* FoundActor = FindObject<AActor>(CurrentLevel, *ActorName);
		if (FoundActor)
		{
			return FoundActor;
		}
	}

	// Fallback: search in all levels by Object Name
	for (ULevel* Level : World->GetLevels())
	{
		if (Level)
		{
			AActor* FoundActor = FindObject<AActor>(Level, *ActorName);
			if (FoundActor)
			{
				return FoundActor;
			}
		}
	}

	// Search by Actor Label
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor)
		{
			// Check Actor Label (exact match)
			if (Actor->GetActorLabel() == ActorName)
			{
				UE_LOG(LogTemp, Display, TEXT("FCommonUtils: Found actor by Label '%s' (ObjectName: %s)"),
					*ActorName, *Actor->GetName());
				return Actor;
			}
		}
	}

	// Partial match on Actor Label
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor)
		{
			// Check if Actor Label contains the search string (case-insensitive)
			if (Actor->GetActorLabel().Contains(ActorName, ESearchCase::IgnoreCase))
			{
				UE_LOG(LogTemp, Display, TEXT("FCommonUtils: Found actor by partial Label match '%s' -> '%s' (ObjectName: %s)"),
					*ActorName, *Actor->GetActorLabel(), *Actor->GetName());
				return Actor;
			}
		}
	}

	return nullptr;
}

AActor* FCommonUtils::FindActorByNameWithAutoLoad(UWorld* World, const FString& ActorName, bool& bOutWasAutoLoaded)
{
	bOutWasAutoLoaded = false;

	if (!World || ActorName.IsEmpty())
	{
		return nullptr;
	}

	// First, try to find the actor among loaded actors
	AActor* FoundActor = FindActorByName(World, ActorName);
	if (FoundActor)
	{
		return FoundActor;
	}

	// Search in Level Instances
	ALevelInstance* OwningLI = nullptr;
	FoundActor = FindActorByNameIncludingLevelInstances(World, ActorName, OwningLI);
	if (FoundActor)
	{
		return FoundActor;
	}

	// If not found and World Partition is enabled, try to load from WP
	UWorldPartition* WorldPartition = World->GetWorldPartition();
	if (WorldPartition)
	{
		FoundActor = TryLoadActorFromWorldPartition(WorldPartition, ActorName);
		if (FoundActor)
		{
			bOutWasAutoLoaded = true;
			UE_LOG(LogTemp, Display, TEXT("FCommonUtils: Auto-loaded actor '%s' from World Partition"), *ActorName);
		}
	}

	return FoundActor;
}

AActor* FCommonUtils::TryLoadActorFromWorldPartition(UWorldPartition* WorldPartition, const FString& ActorName)
{
	if (!WorldPartition || ActorName.IsEmpty())
	{
		return nullptr;
	}

	// Search for the actor in WorldPartition ActorDescs
	FGuid FoundGuid;
	const FWorldPartitionActorDescInstance* FoundDesc = nullptr;
	bool bFoundByLabel = false;
	bool bExactMatchFound = false;

	FWorldPartitionHelpers::ForEachActorDescInstance(WorldPartition, AActor::StaticClass(), [&](const FWorldPartitionActorDescInstance* ActorDescInstance) -> bool
	{
		if (!ActorDescInstance)
		{
			return true; // continue
		}

		// Skip if already loaded
		if (ActorDescInstance->GetActor())
		{
			return true; // continue
		}

		FString DescActorName = ActorDescInstance->GetActorName().ToString();
		FString DescActorLabel = ActorDescInstance->GetActorLabel().ToString();

		// Exact match on Object Name
		if (DescActorName == ActorName)
		{
			FoundGuid = ActorDescInstance->GetGuid();
			FoundDesc = ActorDescInstance;
			bFoundByLabel = false;
			bExactMatchFound = true;
			return false;
		}

		// Exact match on Actor Label
		if (DescActorLabel == ActorName)
		{
			FoundGuid = ActorDescInstance->GetGuid();
			FoundDesc = ActorDescInstance;
			bFoundByLabel = true;
		}

		// Partial match on Object Name
		if (!FoundGuid.IsValid() && DescActorName.Contains(ActorName, ESearchCase::IgnoreCase))
		{
			FoundGuid = ActorDescInstance->GetGuid();
			FoundDesc = ActorDescInstance;
			bFoundByLabel = false;
		}

		// Partial match on Actor Label
		if (!FoundGuid.IsValid() && DescActorLabel.Contains(ActorName, ESearchCase::IgnoreCase))
		{
			FoundGuid = ActorDescInstance->GetGuid();
			FoundDesc = ActorDescInstance;
			bFoundByLabel = true;
		}

		return true; // continue
	});

	if (!FoundGuid.IsValid() || !FoundDesc)
	{
		return nullptr;
	}

	// Check if already loaded
	AActor* ExistingActor = FoundDesc->GetActor();
	if (ExistingActor)
	{
		return ExistingActor;
	}

	// Load actor via reference
	FWorldPartitionReference ActorRef(WorldPartition, FoundGuid);
	if (!ActorRef.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FCommonUtils: Failed to create WorldPartition reference for actor '%s'"), *ActorName);
		return nullptr;
	}

	// Keep reference alive in global storage
	GAutoLoadedActorRefs.Add(MoveTemp(ActorRef));

	// Get the loaded actor
	AActor* LoadedActor = FoundDesc->GetActor();
	if (LoadedActor)
	{
		UE_LOG(LogTemp, Display, TEXT("FCommonUtils: Successfully loaded actor '%s' (Label: '%s', class: %s) from World Partition (matched by %s)"),
			*LoadedActor->GetName(), *LoadedActor->GetActorLabel(), *LoadedActor->GetClass()->GetName(),
			bFoundByLabel ? TEXT("Actor Label") : TEXT("Object Name"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FCommonUtils: Reference created but actor '%s' did not load immediately"), *ActorName);
	}

	return LoadedActor;
}

bool FCommonUtils::SetObjectProperty(UObject* Object, const FString& PropertyName, 
                                     const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
	if (!Object)
	{
		OutErrorMessage = TEXT("Invalid object");
		return false;
	}

	FProperty* Property = Object->GetClass()->FindPropertyByName(*PropertyName);
	if (!Property)
	{
		OutErrorMessage = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
		return false;
	}

	void* PropertyAddr = Property->ContainerPtrToValuePtr<void>(Object);
	
	// Handle different property types
	if (Property->IsA<FBoolProperty>())
	{
		((FBoolProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsBool());
		return true;
	}
	else if (Property->IsA<FIntProperty>())
	{
		int32 IntValue = static_cast<int32>(Value->AsNumber());
		FIntProperty* IntProperty = CastField<FIntProperty>(Property);
		if (IntProperty)
		{
			IntProperty->SetPropertyValue_InContainer(Object, IntValue);
			return true;
		}
	}
	else if (Property->IsA<FFloatProperty>())
	{
		((FFloatProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsNumber());
		return true;
	}
	else if (Property->IsA<FStrProperty>())
	{
		((FStrProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsString());
		return true;
	}
	else if (Property->IsA<FByteProperty>())
	{
		FByteProperty* ByteProp = CastField<FByteProperty>(Property);
		UEnum* EnumDef = ByteProp ? ByteProp->GetIntPropertyEnum() : nullptr;
		
		// If this is a TEnumAsByte property (has associated enum)
		if (EnumDef)
		{
			// Handle numeric value
			if (Value->Type == EJson::Number)
			{
				uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
				ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
				return true;
			}
			// Handle string enum value
			else if (Value->Type == EJson::String)
			{
				FString EnumValueName = Value->AsString();
				
				// Try to convert numeric string to number first
				if (EnumValueName.IsNumeric())
				{
					uint8 ByteValue = FCString::Atoi(*EnumValueName);
					ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
					return true;
				}
				
				// Handle qualified enum names (e.g., "Player0" or "EAutoReceiveInput::Player0")
				if (EnumValueName.Contains(TEXT("::")))
				{
					EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
				}
				
				int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
				if (EnumValue == INDEX_NONE)
				{
					// Try with full name as fallback
					EnumValue = EnumDef->GetValueByNameString(Value->AsString());
				}
				
				if (EnumValue != INDEX_NONE)
				{
					ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(EnumValue));
					return true;
				}
				else
				{
					OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
					return false;
				}
			}
		}
		else
		{
			// Regular byte property
			uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
			ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
			return true;
		}
	}
	else if (Property->IsA<FEnumProperty>())
	{
		FEnumProperty* EnumProp = CastField<FEnumProperty>(Property);
		UEnum* EnumDef = EnumProp ? EnumProp->GetEnum() : nullptr;
		FNumericProperty* UnderlyingNumericProp = EnumProp ? EnumProp->GetUnderlyingProperty() : nullptr;
		
		if (EnumDef && UnderlyingNumericProp)
		{
			// Handle numeric value
			if (Value->Type == EJson::Number)
			{
				int64 EnumValue = static_cast<int64>(Value->AsNumber());
				UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
				return true;
			}
			// Handle string enum value
			else if (Value->Type == EJson::String)
			{
				FString EnumValueName = Value->AsString();
				
				// Try to convert numeric string to number first
				if (EnumValueName.IsNumeric())
				{
					int64 EnumValue = FCString::Atoi64(*EnumValueName);
					UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
					return true;
				}
				
				// Handle qualified enum names
				if (EnumValueName.Contains(TEXT("::")))
				{
					EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
				}
				
				int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
				if (EnumValue == INDEX_NONE)
				{
					// Try with full name as fallback
					EnumValue = EnumDef->GetValueByNameString(Value->AsString());
				}
				
				if (EnumValue != INDEX_NONE)
				{
					UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
					return true;
				}
				else
				{
					OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
					return false;
				}
			}
		}
	}
	
	OutErrorMessage = FString::Printf(TEXT("Unsupported property type: %s for property %s"),
	                                *Property->GetClass()->GetName(), *PropertyName);
	return false;
}

// Blueprint Graph Utilities

UEdGraph* FCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return nullptr;
	}

	// Find existing event graph
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph && Graph->GetFName() == TEXT("EventGraph"))
		{
			return Graph;
		}
	}

	// Create new event graph if not found
	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint,
		TEXT("EventGraph"),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass()
	);

	if (NewGraph)
	{
		FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
	}

	return NewGraph;
}

UK2Node_Event* FCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& NodePosition)
{
	if (!Graph)
	{
		return nullptr;
	}

	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	if (!Blueprint)
	{
		return nullptr;
	}

	// Check for existing event node with this exact name (reuse existing node)
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
		if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
		{
			UE_LOG(LogTemp, Display, TEXT("FCommonUtils: Using existing event node '%s' (ID: %s)"),
				*EventName, *EventNode->NodeGuid.ToString());
			return EventNode;
		}
	}

	// No existing node found, create a new one
	UK2Node_Event* EventNode = nullptr;

	// Find the function to create the event
	UClass* BlueprintClass = Blueprint->GeneratedClass;
	UFunction* EventFunction = BlueprintClass ? BlueprintClass->FindFunctionByName(FName(*EventName)) : nullptr;

	if (EventFunction)
	{
		EventNode = NewObject<UK2Node_Event>(Graph);
		EventNode->CreateNewGuid();
		EventNode->EventReference.SetExternalMember(FName(*EventName), BlueprintClass);
		EventNode->NodePosX = NodePosition.X;
		EventNode->NodePosY = NodePosition.Y;
		Graph->AddNode(EventNode, true);
		EventNode->PostPlacedNewNode();
		EventNode->AllocateDefaultPins();

		UE_LOG(LogTemp, Display, TEXT("FCommonUtils: Created new event node '%s' (ID: %s)"),
			*EventName, *EventNode->NodeGuid.ToString());
	}
	else
	{
		// Fallback: Create event node without function lookup (for standard events like BeginPlay, Tick)
		EventNode = NewObject<UK2Node_Event>(Graph);
		if (EventNode)
		{
			EventNode->CreateNewGuid();
			EventNode->EventReference.SetExternalMember(*EventName, AActor::StaticClass());
			EventNode->bOverrideFunction = true;
			EventNode->NodePosX = NodePosition.X;
			EventNode->NodePosY = NodePosition.Y;
			Graph->AddNode(EventNode, true);
			EventNode->PostPlacedNewNode();
			EventNode->AllocateDefaultPins();

			UE_LOG(LogTemp, Display, TEXT("FCommonUtils: Created event node '%s' (fallback method)"),
				*EventName);
		}
	}

	return EventNode;
}

UK2Node_Event* FCommonUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
	if (!Graph)
	{
		return nullptr;
	}

	// Look for existing event nodes
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
		if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
		{
			UE_LOG(LogTemp, Display, TEXT("FCommonUtils: Found existing event node with name: %s"), *EventName);
			return EventNode;
		}
	}

	return nullptr;
}

UK2Node_CallFunction* FCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& NodePosition)
{
	if (!Graph || !Function)
	{
		return nullptr;
	}

	UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
	if (!FunctionNode)
	{
		return nullptr;
	}

	// 1. Create GUID first
	FunctionNode->CreateNewGuid();

	// 2. Set the function reference (required before AllocateDefaultPins)
	FunctionNode->SetFromFunction(Function);

	// 3. Set node position
	FunctionNode->NodePosX = NodePosition.X;
	FunctionNode->NodePosY = NodePosition.Y;

	// 4. Add to graph
	Graph->AddNode(FunctionNode, true);

	// 5. Post placement and allocate pins
	FunctionNode->PostPlacedNewNode();
	FunctionNode->AllocateDefaultPins();

	return FunctionNode;
}

bool FCommonUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName,
                                       UEdGraphNode* TargetNode, const FString& TargetPinName)
{
	if (!Graph || !SourceNode || !TargetNode)
	{
		return false;
	}

	// Find the pins
	UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
	UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);

	if (!SourcePin || !TargetPin)
	{
		return false;
	}

	// Make the connection
	SourcePin->MakeLinkTo(TargetPin);

	return true;
}

UK2Node_InputAction* FCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& NodePosition)
{
	if (!Graph)
	{
		return nullptr;
	}

	UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
	if (!InputActionNode)
	{
		return nullptr;
	}

	// 1. Create GUID first
	InputActionNode->CreateNewGuid();

	// 2. Set the input action name (required before AllocateDefaultPins)
	InputActionNode->InputActionName = *ActionName;

	// 3. Set node position
	InputActionNode->NodePosX = NodePosition.X;
	InputActionNode->NodePosY = NodePosition.Y;

	// 4. Add to graph
	Graph->AddNode(InputActionNode, true);

	// 5. Post placement and allocate pins
	InputActionNode->PostPlacedNewNode();
	InputActionNode->AllocateDefaultPins();

	return InputActionNode;
}

UK2Node_Self* FCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& NodePosition)
{
	if (!Graph)
	{
		return nullptr;
	}

	UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
	if (!SelfNode)
	{
		return nullptr;
	}

	// 1. Create GUID first
	SelfNode->CreateNewGuid();

	// 2. Set node position
	SelfNode->NodePosX = NodePosition.X;
	SelfNode->NodePosY = NodePosition.Y;

	// 3. Add to graph
	Graph->AddNode(SelfNode, true);

	// 4. Post placement and allocate pins
	SelfNode->PostPlacedNewNode();
	SelfNode->AllocateDefaultPins();

	return SelfNode;
}

UEdGraphPin* FCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
	if (!Node)
	{
		return nullptr;
	}

	// Log all pins for debugging
	UE_LOG(LogTemp, Display, TEXT("FCommonUtils::FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"),
		*PinName, (int32)Direction, *Node->GetName());

	for (UEdGraphPin* Pin : Node->Pins)
	{
		UE_LOG(LogTemp, Verbose, TEXT("  - Available pin: '%s', Direction: %d, Category: %s"),
			*Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
	}

	// First try exact match
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
		{
			UE_LOG(LogTemp, Display, TEXT("  - Found exact matching pin: '%s'"), *Pin->PinName.ToString());
			return Pin;
		}
	}

	// If no exact match, try case-insensitive match
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) &&
			(Direction == EGPD_MAX || Pin->Direction == Direction))
		{
			UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive matching pin: '%s'"), *Pin->PinName.ToString());
			return Pin;
		}
	}

	// If we're looking for a component output and didn't find it by name, try to find the first data output pin
	// This is useful for VariableGet nodes where the output pin name may vary
	if (Direction == EGPD_Output && Cast<UK2Node_VariableGet>(Node) != nullptr)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
			{
				UE_LOG(LogTemp, Display, TEXT("  - Found fallback data output pin: '%s'"), *Pin->PinName.ToString());
				return Pin;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("  - No matching pin found for '%s'"), *PinName);
	return nullptr;
}

UClass* FCommonUtils::FindClassByName(const FString& ClassName)
{
	if (ClassName.IsEmpty())
	{
		return nullptr;
	}

	UClass* FoundClass = nullptr;

	// Try direct lookup
	FoundClass = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::ExactClass);
	if (FoundClass)
	{
		return FoundClass;
	}

	// Try with A prefix (Actor-derived classes)
	if (!ClassName.StartsWith(TEXT("A")))
	{
		FoundClass = FindFirstObject<UClass>(*(TEXT("A") + ClassName), EFindFirstObjectOptions::ExactClass);
		if (FoundClass)
		{
			return FoundClass;
		}
	}

	// Try with U prefix (UObject-derived classes)
	if (!ClassName.StartsWith(TEXT("U")))
	{
		FoundClass = FindFirstObject<UClass>(*(TEXT("U") + ClassName), EFindFirstObjectOptions::ExactClass);
		if (FoundClass)
		{
			return FoundClass;
		}
	}

	// Search in common modules
	static const TArray<FString> Modules = {
		TEXT("/Script/Engine"),
		TEXT("/Script/GameplayAbilities"),
		TEXT("/Script/GameplayTasks"),
		TEXT("/Script/AIModule")
	};

	// Prepare class name variants (with and without U/A prefix)
	TArray<FString> ClassNameVariants;
	ClassNameVariants.Add(ClassName);

	// If starts with U or A, also try without prefix
	if (ClassName.StartsWith(TEXT("U")) && ClassName.Len() > 1)
	{
		ClassNameVariants.Add(ClassName.Mid(1));
	}
	else if (ClassName.StartsWith(TEXT("A")) && ClassName.Len() > 1)
	{
		ClassNameVariants.Add(ClassName.Mid(1));
	}
	else
	{
		// If doesn't start with prefix, also try with prefixes
		ClassNameVariants.Add(TEXT("U") + ClassName);
		ClassNameVariants.Add(TEXT("A") + ClassName);
	}

	for (const FString& Module : Modules)
	{
		for (const FString& TestName : ClassNameVariants)
		{
			FString FullPath = Module + TEXT(".") + TestName;
			FoundClass = LoadClass<UObject>(nullptr, *FullPath);
			if (FoundClass)
			{
				return FoundClass;
			}
		}
	}

	return nullptr;
}

UScriptStruct* FCommonUtils::FindStructByName(const FString& StructName)
{
	if (StructName.IsEmpty())
	{
		return nullptr;
	}

	// If it's a full path, load directly
	if (StructName.Contains(TEXT("/")))
	{
		return LoadObject<UScriptStruct>(nullptr, *StructName);
	}

	// Common struct name to full path mapping
	if (StructName.Equals(TEXT("Transform"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/CoreUObject.Transform"));
	}
	if (StructName.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/CoreUObject.Vector"));
	}
	if (StructName.Equals(TEXT("Rotator"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/CoreUObject.Rotator"));
	}
	if (StructName.Equals(TEXT("LinearColor"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/CoreUObject.LinearColor"));
	}
	if (StructName.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/CoreUObject.Color"));
	}
	if (StructName.Equals(TEXT("Vector2D"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/CoreUObject.Vector2D"));
	}
	if (StructName.Equals(TEXT("HitResult"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/Engine.HitResult"));
	}
	if (StructName.Equals(TEXT("GameplayTag"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/GameplayTags.GameplayTag"));
	}
	if (StructName.Equals(TEXT("GameplayTagContainer"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/GameplayTags.GameplayTagContainer"));
	}
	if (StructName.Equals(TEXT("GameplayEffectSpec"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/GameplayAbilities.GameplayEffectSpec"));
	}
	if (StructName.Equals(TEXT("GameplayAbilitySpec"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/GameplayAbilities.GameplayAbilitySpec"));
	}
	if (StructName.Equals(TEXT("GameplayEventData"), ESearchCase::IgnoreCase))
	{
		return LoadObject<UScriptStruct>(nullptr, TEXT("/Script/GameplayAbilities.GameplayEventData"));
	}

	// Try common modules
	UScriptStruct* FoundStruct = nullptr;

	FoundStruct = FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/CoreUObject.%s"), *StructName));
	if (FoundStruct) return FoundStruct;

	FoundStruct = FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *StructName));
	if (FoundStruct) return FoundStruct;

	FoundStruct = FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/GameplayTags.%s"), *StructName));
	if (FoundStruct) return FoundStruct;

	FoundStruct = FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/GameplayAbilities.%s"), *StructName));
	if (FoundStruct) return FoundStruct;

	return nullptr;
}

UEdGraph* FCommonUtils::CreateFunctionOverride(UBlueprint* Blueprint, const FString& FunctionName, UK2Node_FunctionEntry*& OutFunctionEntry)
{
	OutFunctionEntry = nullptr;

	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("FCommonUtils::CreateFunctionOverride: Blueprint is null"));
		return nullptr;
	}

	// Check if this function override already exists (check both original and K2_ variant)
	TArray<FString> NamesToCheck;
	NamesToCheck.Add(FunctionName);
	if (!FunctionName.StartsWith(TEXT("K2_")))
	{
		NamesToCheck.Add(TEXT("K2_") + FunctionName);
	}

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (!Graph) continue;
		for (const FString& NameToCheck : NamesToCheck)
		{
			if (Graph->GetFName() == FName(*NameToCheck))
			{
				UE_LOG(LogTemp, Display, TEXT("FCommonUtils::CreateFunctionOverride: Found existing override for '%s'"), *NameToCheck);
				// Find the function entry node
				for (UEdGraphNode* Node : Graph->Nodes)
				{
					UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node);
					if (EntryNode)
					{
						OutFunctionEntry = EntryNode;
						return Graph;
					}
				}
				return Graph;
			}
		}
	}

	// Find the function in parent class to override
	UClass* ParentClass = Blueprint->ParentClass;
	if (!ParentClass)
	{
		UE_LOG(LogTemp, Error, TEXT("FCommonUtils::CreateFunctionOverride: Blueprint has no parent class"));
		return nullptr;
	}

	// Try to find function, with K2_ prefix fallback for Blueprint-callable functions
	FString ActualFunctionName = FunctionName;
	UFunction* FunctionToOverride = ParentClass->FindFunctionByName(FName(*FunctionName));
	if (!FunctionToOverride && !FunctionName.StartsWith(TEXT("K2_")))
	{
		FString K2Name = TEXT("K2_") + FunctionName;
		FunctionToOverride = ParentClass->FindFunctionByName(FName(*K2Name));
		if (FunctionToOverride)
		{
			ActualFunctionName = K2Name;
			UE_LOG(LogTemp, Display, TEXT("FCommonUtils::CreateFunctionOverride: Using K2_ variant '%s' instead of '%s'"),
				*K2Name, *FunctionName);
		}
	}
	if (!FunctionToOverride)
	{
		UE_LOG(LogTemp, Error, TEXT("FCommonUtils::CreateFunctionOverride: Function '%s' not found in parent class '%s'"),
			*FunctionName, *ParentClass->GetName());
		return nullptr;
	}

	// Check if function is BlueprintImplementableEvent or BlueprintNativeEvent
	if (!FunctionToOverride->HasAnyFunctionFlags(FUNC_BlueprintEvent))
	{
		UE_LOG(LogTemp, Warning, TEXT("FCommonUtils::CreateFunctionOverride: Function '%s' is not a BlueprintEvent, may not be overridable"),
			*ActualFunctionName);
	}

	// Create the override graph using FBlueprintEditorUtils
	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint,
		FName(*ActualFunctionName),
		UEdGraph::StaticClass(),
		UEdGraphSchema_K2::StaticClass()
	);

	if (!NewGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("FCommonUtils::CreateFunctionOverride: Failed to create graph for '%s'"), *ActualFunctionName);
		return nullptr;
	}

	// Setup the function graph
	FBlueprintEditorUtils::AddFunctionGraph(Blueprint, NewGraph, false, FunctionToOverride);

	// Find the function entry node that was created
	for (UEdGraphNode* Node : NewGraph->Nodes)
	{
		UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node);
		if (EntryNode)
		{
			OutFunctionEntry = EntryNode;
			break;
		}
	}

	// Mark blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	UE_LOG(LogTemp, Display, TEXT("CreateFunctionOverride: Successfully created override for '%s' in Blueprint '%s'"),
		*ActualFunctionName, *Blueprint->GetName());

	return NewGraph;
}

TArray<TSharedPtr<FJsonValue>> FCommonUtils::NodePinsToJson(UEdGraphNode* Node)
{
	TArray<TSharedPtr<FJsonValue>> PinsArray;
	if (!Node) return PinsArray;

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin || Pin->bHidden) continue;

		TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
		PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
		PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
		PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
		PinObj->SetBoolField(TEXT("is_connected"), Pin->LinkedTo.Num() > 0);

		if (Pin->PinType.PinSubCategoryObject.IsValid())
		{
			PinObj->SetStringField(TEXT("sub_type"), Pin->PinType.PinSubCategoryObject->GetName());
		}

		PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
	}

	return PinsArray;
}

TSharedPtr<FJsonObject> FCommonUtils::CreateNodeResponse(UEdGraphNode* Node, bool bSuccess)
{
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), bSuccess);

	if (!Node) return ResultObj;

	ResultObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
	ResultObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
	ResultObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
	ResultObj->SetArrayField(TEXT("pins"), NodePinsToJson(Node));

	if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
	{
		ResultObj->SetStringField(TEXT("event_name"), EventNode->EventReference.GetMemberName().ToString());
	}

	return ResultObj;
}

TSharedPtr<FJsonObject> FInstructionContext::ToJson() const
{
	if (Type.IsEmpty()) return nullptr;

	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("type"), Type);
	Obj->SetStringField(TEXT("message"), Message);

	if (Suggestions.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const FString& S : Suggestions)
		{
			Arr.Add(MakeShared<FJsonValueString>(S));
		}
		Obj->SetArrayField(TEXT("suggestions"), Arr);
	}

	if (!ActionHint.IsEmpty())
	{
		Obj->SetStringField(TEXT("action_hint"), ActionHint);
	}

	return Obj;
}

TSharedPtr<FJsonObject> FCommonUtils::CreateNodeResponseWithContext(UEdGraphNode* Node, const FInstructionContext& Context)
{
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);

	if (!Node) return ResultObj;

	ResultObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
	ResultObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
	ResultObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
	ResultObj->SetArrayField(TEXT("pins"), NodePinsToJson(Node));

	if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
	{
		ResultObj->SetStringField(TEXT("event_name"), EventNode->EventReference.GetMemberName().ToString());
	}

	FInstructionContext FinalContext = Context;
	if (!FinalContext.IsValid())
	{
		TArray<FString> MissingPins = GetUnconnectedRequiredPins(Node);
		if (MissingPins.Num() > 0)
		{
			FinalContext.Type = TEXT("missing_required_pins");
			FinalContext.Message = FString::Printf(TEXT("Required pins not connected: %s"), *FString::Join(MissingPins, TEXT(", ")));
			FinalContext.Suggestions = MissingPins;
			FinalContext.ActionHint = TEXT("Use 'connect_blueprint_nodes' to connect these pins.");
		}
	}

	if (FinalContext.IsValid())
	{
		ResultObj->SetObjectField(TEXT("instruction"), FinalContext.ToJson());
	}

	return ResultObj;
}

TSharedPtr<FJsonObject> FCommonUtils::CreateErrorWithInstruction(const FString& ErrorMessage, const FInstructionContext& Context)
{
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), false);
	ResultObj->SetStringField(TEXT("error"), ErrorMessage);

	if (Context.IsValid())
	{
		ResultObj->SetObjectField(TEXT("instruction"), Context.ToJson());
	}

	return ResultObj;
}

EPinRequirement FCommonUtils::GetPinRequirement(UEdGraphPin* Pin)
{
	if (!Pin || Pin->bHidden || Pin->bNotConnectable || Pin->bOrphanedPin)
	{
		return EPinRequirement::NotConnectable;
	}

	// Exec pins
	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
	{
		if (Pin->Direction == EGPD_Input)
		{
			// Event/FunctionEntry nodes don't need input exec connection
			UEdGraphNode* OwnerNode = Pin->GetOwningNode();
			if (Cast<UK2Node_Event>(OwnerNode) || Cast<UK2Node_FunctionEntry>(OwnerNode))
			{
				return EPinRequirement::NotConnectable;
			}
			return EPinRequirement::Required;
		}
		return EPinRequirement::Optional;
	}

	// Output data pins are always optional
	if (Pin->Direction == EGPD_Output)
	{
		return EPinRequirement::Optional;
	}

	// Already connected
	if (Pin->LinkedTo.Num() > 0)
	{
		return EPinRequirement::Optional;
	}

	// Has default value
	if (!Pin->DefaultValue.IsEmpty() || !Pin->AutogeneratedDefaultValue.IsEmpty() || Pin->bDefaultValueIsIgnored)
	{
		return EPinRequirement::Optional;
	}

	return EPinRequirement::Required;
}

TArray<FString> FCommonUtils::FindSimilarNames(const FString& Input, const TArray<FString>& Candidates, int32 MaxResults)
{
	TArray<FString> Results;

	for (const FString& Candidate : Candidates)
	{
		if (Candidate.Contains(Input, ESearchCase::IgnoreCase) ||
			Input.Contains(Candidate, ESearchCase::IgnoreCase))
		{
			Results.Add(Candidate);
			if (Results.Num() >= MaxResults) break;
		}
	}

	return Results;
}

TArray<FString> FCommonUtils::GetUnconnectedRequiredPins(UEdGraphNode* Node)
{
	TArray<FString> MissingPins;
	if (!Node) return MissingPins;

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (GetPinRequirement(Pin) == EPinRequirement::Required && Pin->LinkedTo.Num() == 0)
		{
			MissingPins.Add(Pin->PinName.ToString());
		}
	}

	return MissingPins;
}

TArray<TSharedPtr<FJsonValue>> FCommonUtils::ValidateBlueprintGraphs(UBlueprint* Blueprint)
{
	TArray<TSharedPtr<FJsonValue>> Issues;
	if (!Blueprint) return Issues;

	TArray<UEdGraph*> AllGraphs = GetAllGraphs(Blueprint);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph) continue;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;

			TArray<FString> MissingPins = GetUnconnectedRequiredPins(Node);
			for (const FString& PinName : MissingPins)
			{
				TSharedPtr<FJsonObject> IssueObj = MakeShared<FJsonObject>();
				IssueObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
				IssueObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
				IssueObj->SetStringField(TEXT("issue_type"), TEXT("unconnected_required_pin"));
				IssueObj->SetStringField(TEXT("pin_name"), PinName);
				IssueObj->SetStringField(TEXT("graph_name"), Graph->GetName());
				Issues.Add(MakeShared<FJsonValueObject>(IssueObj));
			}
		}
	}

	return Issues;
}

// ============================================================================
// Graph Router
// ============================================================================

UEdGraph* FCommonUtils::FindGraphByName(UBlueprint* Blueprint, const FString& GraphName)
{
	if (!Blueprint || GraphName.IsEmpty())
	{
		return nullptr;
	}

	// Search UbergraphPages (EventGraph, etc.)
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph && Graph->GetName() == GraphName)
		{
			return Graph;
		}
	}

	// Search FunctionGraphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph && Graph->GetName() == GraphName)
		{
			return Graph;
		}
	}

	// Search MacroGraphs
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		if (Graph && Graph->GetName() == GraphName)
		{
			return Graph;
		}
	}

	// Search DelegateSignatureGraphs
	for (UEdGraph* Graph : Blueprint->DelegateSignatureGraphs)
	{
		if (Graph && Graph->GetName() == GraphName)
		{
			return Graph;
		}
	}

	// Default: return EventGraph if name matches common aliases
	if (GraphName.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase) ||
		GraphName.Equals(TEXT("event_graph"), ESearchCase::IgnoreCase) ||
		GraphName.IsEmpty())
	{
		return FindOrCreateEventGraph(Blueprint);
	}

	return nullptr;
}

TArray<UEdGraph*> FCommonUtils::GetAllGraphs(UBlueprint* Blueprint)
{
	TArray<UEdGraph*> AllGraphs;
	if (!Blueprint) return AllGraphs;

	AllGraphs.Append(Blueprint->UbergraphPages);
	AllGraphs.Append(Blueprint->FunctionGraphs);
	AllGraphs.Append(Blueprint->MacroGraphs);
	AllGraphs.Append(Blueprint->DelegateSignatureGraphs);

	return AllGraphs;
}

TSharedPtr<FJsonObject> FCommonUtils::GraphToJson(UEdGraph* Graph)
{
	TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
	if (!Graph) return GraphObj;

	GraphObj->SetStringField(TEXT("name"), Graph->GetName());
	GraphObj->SetStringField(TEXT("schema"), Graph->Schema ? Graph->Schema->GetClass()->GetName() : TEXT("None"));

	// Determine graph type
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	FString GraphType = TEXT("Unknown");
	if (Blueprint)
	{
		if (Blueprint->UbergraphPages.Contains(Graph)) GraphType = TEXT("Ubergraph");
		else if (Blueprint->FunctionGraphs.Contains(Graph)) GraphType = TEXT("Function");
		else if (Blueprint->MacroGraphs.Contains(Graph)) GraphType = TEXT("Macro");
		else if (Blueprint->DelegateSignatureGraphs.Contains(Graph)) GraphType = TEXT("Delegate");
	}
	GraphObj->SetStringField(TEXT("type"), GraphType);
	GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

	return GraphObj;
}

// ============================================================================
// Generic Node Factory
// ============================================================================

static UClass* FindNodeClassByName(const FString& NodeClassName)
{
	UClass* NodeClass = nullptr;

	// Try direct lookup
	NodeClass = FindFirstObject<UClass>(*NodeClassName, EFindFirstObjectOptions::ExactClass);

	// Try with UK2Node_ prefix
	if (!NodeClass && !NodeClassName.StartsWith(TEXT("UK2Node_")) && !NodeClassName.StartsWith(TEXT("K2Node_")))
	{
		NodeClass = FindFirstObject<UClass>(*(TEXT("UK2Node_") + NodeClassName), EFindFirstObjectOptions::ExactClass);
		if (!NodeClass)
		{
			NodeClass = FindFirstObject<UClass>(*(TEXT("K2Node_") + NodeClassName), EFindFirstObjectOptions::ExactClass);
		}
	}

	// Search in BlueprintGraph module
	if (!NodeClass)
	{
		FString TestName = NodeClassName;
		if (!TestName.StartsWith(TEXT("K2Node_")) && !TestName.StartsWith(TEXT("UK2Node_")))
		{
			TestName = TEXT("K2Node_") + NodeClassName;
		}
		FString FullPath = FString::Printf(TEXT("/Script/BlueprintGraph.%s"), *TestName);
		NodeClass = LoadClass<UEdGraphNode>(nullptr, *FullPath);
	}

	// Try GameplayAbilities module for GAS nodes
	if (!NodeClass)
	{
		FString TestName = NodeClassName;
		if (!TestName.StartsWith(TEXT("K2Node_")))
		{
			TestName = TEXT("K2Node_") + NodeClassName;
		}
		FString GameplayAbilitiesPath = FString::Printf(TEXT("/Script/GameplayAbilities.%s"), *TestName);
		NodeClass = LoadClass<UEdGraphNode>(nullptr, *GameplayAbilitiesPath);
	}

	// Try GameplayAbilitiesEditor module
	if (!NodeClass)
	{
		FString TestName = NodeClassName;
		if (!TestName.StartsWith(TEXT("K2Node_")))
		{
			TestName = TEXT("K2Node_") + NodeClassName;
		}
		FString GameplayAbilitiesEditorPath = FString::Printf(TEXT("/Script/GameplayAbilitiesEditor.%s"), *TestName);
		NodeClass = LoadClass<UEdGraphNode>(nullptr, *GameplayAbilitiesEditorPath);
	}

	return NodeClass;
}

UEdGraphNode* FCommonUtils::CreateNodeByClassName(UEdGraph* Graph, const FString& NodeClassName, const FVector2D& Position)
{
	if (!Graph || NodeClassName.IsEmpty())
	{
		return nullptr;
	}

	// Find node class
	UClass* NodeClass = FindNodeClassByName(NodeClassName);

	if (!NodeClass || !NodeClass->IsChildOf(UEdGraphNode::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("FCommonUtils::CreateNodeByClassName: Class '%s' not found or not a graph node"), *NodeClassName);
		return nullptr;
	}

	// Use UBlueprintNodeSpawner for safe node creation (handles PreInit automatically)
	UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(NodeClass);
	if (!Spawner)
	{
		UE_LOG(LogTemp, Warning, TEXT("FCommonUtils::CreateNodeByClassName: Failed to create spawner for '%s'"), *NodeClassName);
		return nullptr;
	}

	IBlueprintNodeBinder::FBindingSet Bindings;
	UEdGraphNode* NewNode = Spawner->Invoke(Graph, Bindings, Position);

	if (!NewNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("FCommonUtils::CreateNodeByClassName: Spawner failed to create node '%s'"), *NodeClassName);
		return nullptr;
	}

	return NewNode;
}

bool FCommonUtils::InitializeNodeFromParams(UEdGraphNode* Node, const TSharedPtr<FJsonObject>& Params)
{
	if (!Node || !Params.IsValid())
	{
		return false;
	}

	bool bSuccess = true;
	bool bAnyPropertySet = false;

	// Skip reserved and pre-init keys
	static const TSet<FString> SkipKeys = {
		TEXT("node_class"), TEXT("node_position"), TEXT("graph_name"),
		TEXT("blueprint_name"), TEXT("blueprint_path"),
		// PreInit keys (already handled before AllocateDefaultPins)
		TEXT("DataTable"), TEXT("data_table"),
		TEXT("StructType"), TEXT("struct_type"),
		TEXT("Enum"), TEXT("enum"),
		TEXT("TargetType"), TEXT("target_type"),
		TEXT("ActorClass"), TEXT("actor_class"),
		TEXT("ComponentClass"), TEXT("component_class"),
		TEXT("Class"), TEXT("class")
	};

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Params->Values)
	{
		if (SkipKeys.Contains(Pair.Key))
		{
			continue;
		}

		FString OutError;
		if (SetNodePropertyByPath(Node, Pair.Key, Pair.Value, OutError))
		{
			bAnyPropertySet = true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FCommonUtils::InitializeNodeFromParams: Failed to set '%s': %s"), *Pair.Key, *OutError);
			bSuccess = false;
		}
	}

	// Reconstruct only if properties were changed
	if (bAnyPropertySet)
	{
		Node->ReconstructNode();
	}

	return bSuccess;
}

// ============================================================================
// Reflection-based Property Setter
// ============================================================================

bool FCommonUtils::SetNodePropertyByPath(UEdGraphNode* Node, const FString& PropertyPath,
                                          const TSharedPtr<FJsonValue>& Value, FString& OutError)
{
	if (!Node || PropertyPath.IsEmpty() || !Value.IsValid())
	{
		OutError = TEXT("Invalid parameters");
		return false;
	}

	// Split property path (e.g., "EventReference.MemberName")
	TArray<FString> PathParts;
	PropertyPath.ParseIntoArray(PathParts, TEXT("."), true);

	UObject* CurrentObject = Node;
	FProperty* CurrentProperty = nullptr;
	void* CurrentContainer = Node;

	// Navigate to target property
	for (int32 i = 0; i < PathParts.Num(); ++i)
	{
		const FString& Part = PathParts[i];
		CurrentProperty = CurrentObject->GetClass()->FindPropertyByName(*Part);

		if (!CurrentProperty)
		{
			OutError = FString::Printf(TEXT("Property '%s' not found in path '%s'"), *Part, *PropertyPath);
			return false;
		}

		// If not the last part, navigate into struct
		if (i < PathParts.Num() - 1)
		{
			FStructProperty* StructProp = CastField<FStructProperty>(CurrentProperty);
			if (!StructProp)
			{
				OutError = FString::Printf(TEXT("Property '%s' is not a struct, cannot navigate further"), *Part);
				return false;
			}
			CurrentContainer = StructProp->ContainerPtrToValuePtr<void>(CurrentContainer);
		}
	}

	if (!CurrentProperty)
	{
		OutError = FString::Printf(TEXT("Failed to resolve property path '%s'"), *PropertyPath);
		return false;
	}

	// Set value based on property type
	void* PropertyAddr = CurrentProperty->ContainerPtrToValuePtr<void>(CurrentContainer);

	if (FNameProperty* NameProp = CastField<FNameProperty>(CurrentProperty))
	{
		NameProp->SetPropertyValue(PropertyAddr, FName(*Value->AsString()));
		return true;
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(CurrentProperty))
	{
		StrProp->SetPropertyValue(PropertyAddr, Value->AsString());
		return true;
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(CurrentProperty))
	{
		BoolProp->SetPropertyValue(PropertyAddr, Value->AsBool());
		return true;
	}
	else if (FIntProperty* IntProp = CastField<FIntProperty>(CurrentProperty))
	{
		IntProp->SetPropertyValue(PropertyAddr, static_cast<int32>(Value->AsNumber()));
		return true;
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(CurrentProperty))
	{
		FloatProp->SetPropertyValue(PropertyAddr, static_cast<float>(Value->AsNumber()));
		return true;
	}
	else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(CurrentProperty))
	{
		DoubleProp->SetPropertyValue(PropertyAddr, Value->AsNumber());
		return true;
	}
	// UObject* property (asset path string)
	else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(CurrentProperty))
	{
		FString ObjectPath = Value->AsString();
		UObject* LoadedObject = LoadObject<UObject>(nullptr, *ObjectPath);
		if (LoadedObject)
		{
			ObjProp->SetObjectPropertyValue(PropertyAddr, LoadedObject);
			return true;
		}
		OutError = FString::Printf(TEXT("Failed to load object: %s"), *ObjectPath);
		return false;
	}
	// UClass* property (class path string)
	else if (FClassProperty* ClassProp = CastField<FClassProperty>(CurrentProperty))
	{
		FString ClassPath = Value->AsString();
		UClass* LoadedClass = LoadClass<UObject>(nullptr, *ClassPath);
		if (LoadedClass)
		{
			ClassProp->SetObjectPropertyValue(PropertyAddr, LoadedClass);
			return true;
		}
		OutError = FString::Printf(TEXT("Failed to load class: %s"), *ClassPath);
		return false;
	}
	// Enum property (string or int)
	else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(CurrentProperty))
	{
		UEnum* Enum = EnumProp->GetEnum();
		if (Enum)
		{
			if (Value->Type == EJson::String)
			{
				int64 EnumValue = Enum->GetValueByNameString(Value->AsString());
				if (EnumValue != INDEX_NONE)
				{
					EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(PropertyAddr, EnumValue);
					return true;
				}
			}
			else if (Value->Type == EJson::Number)
			{
				EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(PropertyAddr, static_cast<int64>(Value->AsNumber()));
				return true;
			}
		}
		OutError = FString::Printf(TEXT("Invalid enum value for '%s'"), *PropertyPath);
		return false;
	}
	// Byte enum property
	else if (FByteProperty* ByteProp = CastField<FByteProperty>(CurrentProperty))
	{
		if (ByteProp->Enum)
		{
			if (Value->Type == EJson::String)
			{
				int64 EnumValue = ByteProp->Enum->GetValueByNameString(Value->AsString());
				if (EnumValue != INDEX_NONE)
				{
					ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(EnumValue));
					return true;
				}
			}
			else if (Value->Type == EJson::Number)
			{
				ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(Value->AsNumber()));
				return true;
			}
		}
		else
		{
			ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(Value->AsNumber()));
			return true;
		}
	}

	OutError = FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyPath);
	return false;
}

TSharedPtr<FJsonValue> FCommonUtils::GetNodePropertyByPath(UEdGraphNode* Node, const FString& PropertyPath)
{
	if (!Node || PropertyPath.IsEmpty())
	{
		return nullptr;
	}

	TArray<FString> PathParts;
	PropertyPath.ParseIntoArray(PathParts, TEXT("."), true);

	UObject* CurrentObject = Node;
	void* CurrentContainer = Node;
	FProperty* CurrentProperty = nullptr;

	for (int32 i = 0; i < PathParts.Num(); ++i)
	{
		const FString& Part = PathParts[i];
		CurrentProperty = CurrentObject->GetClass()->FindPropertyByName(*Part);

		if (!CurrentProperty) return nullptr;

		if (i < PathParts.Num() - 1)
		{
			FStructProperty* StructProp = CastField<FStructProperty>(CurrentProperty);
			if (!StructProp) return nullptr;
			CurrentContainer = StructProp->ContainerPtrToValuePtr<void>(CurrentContainer);
		}
	}

	if (!CurrentProperty) return nullptr;

	void* PropertyAddr = CurrentProperty->ContainerPtrToValuePtr<void>(CurrentContainer);

	if (FNameProperty* NameProp = CastField<FNameProperty>(CurrentProperty))
	{
		return MakeShared<FJsonValueString>(NameProp->GetPropertyValue(PropertyAddr).ToString());
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(CurrentProperty))
	{
		return MakeShared<FJsonValueString>(StrProp->GetPropertyValue(PropertyAddr));
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(CurrentProperty))
	{
		return MakeShared<FJsonValueBoolean>(BoolProp->GetPropertyValue(PropertyAddr));
	}
	else if (FIntProperty* IntProp = CastField<FIntProperty>(CurrentProperty))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(IntProp->GetPropertyValue(PropertyAddr)));
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(CurrentProperty))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(FloatProp->GetPropertyValue(PropertyAddr)));
	}

	return nullptr;
}

// ============================================================================
// Schema-based Auto Connection
// ============================================================================

bool FCommonUtils::TryAutoConnectNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, UEdGraphNode* TargetNode,
                                        bool bConnectExec, bool bConnectData)
{
	if (!Graph || !SourceNode || !TargetNode)
	{
		return false;
	}

	const UEdGraphSchema* Schema = Graph->GetSchema();
	if (!Schema)
	{
		return false;
	}

	bool bAnyConnected = false;

	// Connect exec pins
	if (bConnectExec)
	{
		UEdGraphPin* SourceExec = FindFirstUnconnectedPin(SourceNode, EGPD_Output, UEdGraphSchema_K2::PC_Exec);
		UEdGraphPin* TargetExec = FindFirstUnconnectedPin(TargetNode, EGPD_Input, UEdGraphSchema_K2::PC_Exec);

		if (SourceExec && TargetExec)
		{
			FPinConnectionResponse Response = Schema->CanCreateConnection(SourceExec, TargetExec);
			if (Response.Response == CONNECT_RESPONSE_MAKE || Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_A ||
				Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_B || Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_AB)
			{
				Schema->TryCreateConnection(SourceExec, TargetExec);
				bAnyConnected = true;
			}
		}
	}

	// Connect data pins (find compatible pairs)
	if (bConnectData)
	{
		for (UEdGraphPin* SourcePin : SourceNode->Pins)
		{
			if (!SourcePin || SourcePin->bHidden || SourcePin->Direction != EGPD_Output ||
				SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec ||
				SourcePin->LinkedTo.Num() > 0)
			{
				continue;
			}

			for (UEdGraphPin* TargetPin : TargetNode->Pins)
			{
				if (!TargetPin || TargetPin->bHidden || TargetPin->Direction != EGPD_Input ||
					TargetPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec ||
					TargetPin->LinkedTo.Num() > 0)
				{
					continue;
				}

				FPinConnectionResponse Response = Schema->CanCreateConnection(SourcePin, TargetPin);
				if (Response.Response == CONNECT_RESPONSE_MAKE)
				{
					Schema->TryCreateConnection(SourcePin, TargetPin);
					bAnyConnected = true;
					break;
				}
			}
		}
	}

	return bAnyConnected;
}

UEdGraphPin* FCommonUtils::FindFirstUnconnectedPin(UEdGraphNode* Node, EEdGraphPinDirection Direction, FName PinCategory)
{
	if (!Node) return nullptr;

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!Pin || Pin->bHidden || Pin->Direction != Direction || Pin->LinkedTo.Num() > 0)
		{
			continue;
		}

		if (PinCategory != NAME_None && Pin->PinType.PinCategory != PinCategory)
		{
			continue;
		}

		return Pin;
	}

	return nullptr;
}

// ============================================================================
// Node Search by GUID
// ============================================================================

UEdGraphNode* FCommonUtils::FindNodeByGuid(UEdGraph* Graph, const FString& NodeGuid)
{
	if (!Graph || NodeGuid.IsEmpty()) return nullptr;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && Node->NodeGuid.ToString() == NodeGuid)
		{
			return Node;
		}
	}

	return nullptr;
}

UEdGraphNode* FCommonUtils::FindNodeByGuidInBlueprint(UBlueprint* Blueprint, const FString& NodeGuid)
{
	if (!Blueprint || NodeGuid.IsEmpty()) return nullptr;

	TArray<UEdGraph*> AllGraphs = GetAllGraphs(Blueprint);

	for (UEdGraph* Graph : AllGraphs)
	{
		UEdGraphNode* Node = FindNodeByGuid(Graph, NodeGuid);
		if (Node) return Node;
	}

	return nullptr;
}

// ============================================================================
// Level Instance Utilities
// ============================================================================

ULevel* FCommonUtils::GetLevelInstanceLoadedLevel(ALevelInstance* LevelInstance)
{
	if (!LevelInstance)
	{
		return nullptr;
	}

	if (ILevelInstanceInterface* LIInterface = Cast<ILevelInstanceInterface>(LevelInstance))
	{
		return LIInterface->GetLoadedLevel();
	}
	return nullptr;
}

TArray<ALevelInstance*> FCommonUtils::GetAllLevelInstances(UWorld* World)
{
	TArray<ALevelInstance*> Result;
	if (!World)
	{
		return Result;
	}

	for (TActorIterator<ALevelInstance> It(World); It; ++It)
	{
		if (*It)
		{
			Result.Add(*It);
		}
	}
	return Result;
}

void FCommonUtils::ForEachActorInWorld(UWorld* World, TFunction<bool(AActor*, ALevelInstance*)> Callback, bool bIncludeLevelInstances)
{
	if (!World)
	{
		return;
	}

	// Iterate main world actors
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		// Skip internal LevelInstance editor actors
		if (Actor->IsA<ALevelInstanceEditorInstanceActor>())
		{
			continue;
		}

		if (!Callback(Actor, nullptr))
		{
			return;
		}
	}

	// Traverse Level Instances if requested
	if (bIncludeLevelInstances)
	{
		for (TActorIterator<ALevelInstance> LIIt(World); LIIt; ++LIIt)
		{
			ALevelInstance* LevelInstance = *LIIt;
			if (!LevelInstance)
			{
				continue;
			}

			ULevel* LoadedLevel = GetLevelInstanceLoadedLevel(LevelInstance);
			if (!LoadedLevel)
			{
				continue;
			}

			for (AActor* Actor : LoadedLevel->Actors)
			{
				if (!Actor)
				{
					continue;
				}

				if (Actor->IsA<ALevelInstanceEditorInstanceActor>())
				{
					continue;
				}

				if (!Callback(Actor, LevelInstance))
				{
					return;
				}
			}
		}
	}
}

AActor* FCommonUtils::FindActorByNameIncludingLevelInstances(UWorld* World, const FString& ActorName, ALevelInstance*& OutOwningLevelInstance)
{
	OutOwningLevelInstance = nullptr;

	if (!World || ActorName.IsEmpty())
	{
		return nullptr;
	}

	// First try standard search (main level actors)
	AActor* Found = FindActorByName(World, ActorName);
	if (Found)
	{
		return Found;
	}

	// Search in Level Instances
	AActor* Result = nullptr;
	ForEachActorInWorld(World, [&](AActor* Actor, ALevelInstance* OwningLI) -> bool
	{
		// Skip main level actors (already searched above)
		if (!OwningLI)
		{
			return true;
		}

		// Check by Object Name (exact)
		if (Actor->GetName() == ActorName)
		{
			Result = Actor;
			OutOwningLevelInstance = OwningLI;
			return false;
		}

		// Check by Actor Label (exact)
		if (Actor->GetActorLabel() == ActorName)
		{
			Result = Actor;
			OutOwningLevelInstance = OwningLI;
			return false;
		}

		// Partial match on label (case-insensitive)
		if (Actor->GetActorLabel().Contains(ActorName, ESearchCase::IgnoreCase))
		{
			Result = Actor;
			OutOwningLevelInstance = OwningLI;
			return false;
		}

		return true;
	}, true);

	if (Result)
	{
		UE_LOG(LogTemp, Display, TEXT("FCommonUtils: Found actor '%s' inside Level Instance '%s'"),
			*ActorName, *OutOwningLevelInstance->GetName());
	}

	return Result;
}
