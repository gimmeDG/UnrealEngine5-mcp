#include "Commands/PCGCommands.h"
#include "Commands/CommonUtils.h"
#include "Dom/JsonObject.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGEdge.h"
#include "PCGSettings.h"
#include "PCGComponent.h"
#include "PCGPin.h"

FPCGCommands::FPCGCommands()
{
}

FPCGCommands::~FPCGCommands()
{
}

TSharedPtr<FJsonObject> FPCGCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	// PCG Graph asset commands
	if (CommandType == TEXT("create_pcg_graph"))
	{
		return HandleCreatePCGGraph(Params);
	}
	else if (CommandType == TEXT("analyze_pcg_graph"))
	{
		return HandleAnalyzePCGGraph(Params);
	}
	else if (CommandType == TEXT("set_pcg_graph_to_component"))
	{
		return HandleSetPCGGraphToComponent(Params);
	}
	// PCG node creation commands
	else if (CommandType == TEXT("add_pcg_sampler_node"))
	{
		return HandleAddPCGSamplerNode(Params);
	}
	else if (CommandType == TEXT("add_pcg_filter_node"))
	{
		return HandleAddPCGFilterNode(Params);
	}
	else if (CommandType == TEXT("add_pcg_transform_node"))
	{
		return HandleAddPCGTransformNode(Params);
	}
	else if (CommandType == TEXT("add_pcg_spawner_node"))
	{
		return HandleAddPCGSpawnerNode(Params);
	}
	else if (CommandType == TEXT("add_pcg_attribute_node"))
	{
		return HandleAddPCGAttributeNode(Params);
	}
	else if (CommandType == TEXT("add_pcg_flow_control_node"))
	{
		return HandleAddPCGFlowControlNode(Params);
	}
	else if (CommandType == TEXT("add_pcg_generic_node"))
	{
		return HandleAddPCGGenericNode(Params);
	}
	// PCG node list commands
	else if (CommandType == TEXT("list_pcg_nodes"))
	{
		return HandleListPCGNodes(Params);
	}
	// PCG node connection commands
	else if (CommandType == TEXT("connect_pcg_nodes"))
	{
		return HandleConnectPCGNodes(Params);
	}
	else if (CommandType == TEXT("disconnect_pcg_nodes"))
	{
		return HandleDisconnectPCGNodes(Params);
	}
	// PCG node deletion
	else if (CommandType == TEXT("delete_pcg_node"))
	{
		return HandleDeletePCGNode(Params);
	}

	return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown PCG command: %s"), *CommandType));
}

// ============================================================================
// Helper Methods
// ============================================================================

UPCGGraph* FPCGCommands::FindPCGGraph(const FString& GraphName, const FString& GraphPath)
{
	// Search all content paths when "/" specified
	if (GraphPath == TEXT("/"))
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		FARFilter Filter;
		Filter.ClassPaths.Add(UPCGGraph::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> AssetList;
		AssetRegistry.GetAssets(Filter, AssetList);

		for (const FAssetData& Asset : AssetList)
		{
			if (Asset.AssetName.ToString() == GraphName)
			{
				return Cast<UPCGGraph>(Asset.GetAsset());
			}
		}
		return nullptr;
	}

	FString FullPath = GraphPath;
	if (!FullPath.EndsWith(TEXT("/")))
	{
		FullPath += TEXT("/");
	}
	FullPath += GraphName;

	UPCGGraph* Graph = Cast<UPCGGraph>(UEditorAssetLibrary::LoadAsset(FullPath));
	if (!Graph)
	{
		Graph = Cast<UPCGGraph>(UEditorAssetLibrary::LoadAsset(FullPath + TEXT(".") + GraphName));
	}

	return Graph;
}

UPCGNode* FPCGCommands::CreatePCGNode(UPCGGraph* Graph, const FString& SettingsClassName, const FVector2D& Position)
{
	if (!Graph)
	{
		UE_LOG(LogTemp, Error, TEXT("FPCGCommands::CreatePCGNode: Graph is null"));
		return nullptr;
	}

	if (SettingsClassName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("FPCGCommands::CreatePCGNode: SettingsClassName is empty"));
		return nullptr;
	}

	// Find settings class
	UClass* SettingsClass = nullptr;

	// Try direct lookup
	SettingsClass = FindFirstObject<UClass>(*SettingsClassName, EFindFirstObjectOptions::ExactClass);

	// Try with UPCG prefix and Settings suffix
	if (!SettingsClass)
	{
		FString TestName = SettingsClassName;
		if (!TestName.StartsWith(TEXT("UPCG")) && !TestName.StartsWith(TEXT("PCG")))
		{
			TestName = TEXT("PCG") + TestName;
		}
		if (!TestName.EndsWith(TEXT("Settings")))
		{
			TestName += TEXT("Settings");
		}
		SettingsClass = FindFirstObject<UClass>(*TestName, EFindFirstObjectOptions::ExactClass);

		if (!SettingsClass)
		{
			SettingsClass = FindFirstObject<UClass>(*(TEXT("U") + TestName), EFindFirstObjectOptions::ExactClass);
		}
	}

	// Try loading from PCG module
	if (!SettingsClass)
	{
		FString ModulePath = FString::Printf(TEXT("/Script/PCG.%s"), *SettingsClassName);
		SettingsClass = LoadClass<UPCGSettings>(nullptr, *ModulePath);
	}

	if (!SettingsClass || !SettingsClass->IsChildOf(UPCGSettings::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("FPCGCommands::CreatePCGNode: Settings class '%s' not found or not derived from UPCGSettings"), *SettingsClassName);
		return nullptr;
	}

	// Create settings instance
	UPCGSettings* Settings = NewObject<UPCGSettings>(Graph, SettingsClass);
	if (!Settings)
	{
		UE_LOG(LogTemp, Error, TEXT("FPCGCommands::CreatePCGNode: Failed to create settings instance for class '%s'"), *SettingsClassName);
		return nullptr;
	}

	// Add node to graph
	UPCGNode* NewNode = Graph->AddNode(Settings);
	if (NewNode)
	{
		NewNode->PositionX = Position.X;
		NewNode->PositionY = Position.Y;
	}

	return NewNode;
}

TSharedPtr<FJsonObject> FPCGCommands::CreatePCGNodeResponse(UPCGNode* Node)
{
	if (!Node)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Node is null"));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("node_id"), Node->GetFName().ToString());
	ResultObj->SetNumberField(TEXT("position_x"), Node->PositionX);
	ResultObj->SetNumberField(TEXT("position_y"), Node->PositionY);

	// Get settings info
	if (UPCGSettings* Settings = Node->GetSettings())
	{
		ResultObj->SetStringField(TEXT("settings_class"), Settings->GetClass()->GetName());
	}

	// Get input pins
	TArray<TSharedPtr<FJsonValue>> InputPinsArray;
	for (UPCGPin* Pin : Node->GetInputPins())
	{
		if (Pin)
		{
			TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
			PinObj->SetStringField(TEXT("name"), Pin->Properties.Label.ToString());
			PinObj->SetBoolField(TEXT("is_connected"), Pin->EdgeCount() > 0);
			InputPinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
		}
	}
	ResultObj->SetArrayField(TEXT("input_pins"), InputPinsArray);

	// Get output pins
	TArray<TSharedPtr<FJsonValue>> OutputPinsArray;
	for (UPCGPin* Pin : Node->GetOutputPins())
	{
		if (Pin)
		{
			TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
			PinObj->SetStringField(TEXT("name"), Pin->Properties.Label.ToString());
			PinObj->SetBoolField(TEXT("is_connected"), Pin->EdgeCount() > 0);
			OutputPinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
		}
	}
	ResultObj->SetArrayField(TEXT("output_pins"), OutputPinsArray);

	return ResultObj;
}

// ============================================================================
// PCG Graph Asset Commands
// ============================================================================

TSharedPtr<FJsonObject> FPCGCommands::HandleCreatePCGGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString PackagePath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("path"), PackagePath);

	if (!PackagePath.EndsWith(TEXT("/")))
	{
		PackagePath += TEXT("/");
	}

	// Check if graph already exists
	FString FullPath = PackagePath + GraphName;
	if (UEditorAssetLibrary::DoesAssetExist(FullPath))
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph already exists: %s"), *GraphName));
	}

	// Create package
	UPackage* Package = CreatePackage(*(PackagePath + GraphName));
	if (!Package)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
	}

	// Create PCG Graph
	UPCGGraph* NewGraph = NewObject<UPCGGraph>(Package, *GraphName, RF_Public | RF_Standalone);
	if (!NewGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create PCG Graph"));
	}

	// Register with asset registry
	FAssetRegistryModule::AssetCreated(NewGraph);
	Package->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("name"), GraphName);
	ResultObj->SetStringField(TEXT("path"), FullPath);
	return ResultObj;
}

TSharedPtr<FJsonObject> FPCGCommands::HandleAnalyzePCGGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("name"), GraphName);

	// Collect all nodes
	TArray<TSharedPtr<FJsonValue>> NodesArray;
	for (UPCGNode* Node : Graph->GetNodes())
	{
		if (!Node)
		{
			continue;
		}

		TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
		NodeObj->SetStringField(TEXT("node_id"), Node->GetFName().ToString());
		NodeObj->SetNumberField(TEXT("position_x"), Node->PositionX);
		NodeObj->SetNumberField(TEXT("position_y"), Node->PositionY);

		if (UPCGSettings* Settings = Node->GetSettings())
		{
			NodeObj->SetStringField(TEXT("settings_class"), Settings->GetClass()->GetName());
		}

		// Input pins
		TArray<TSharedPtr<FJsonValue>> InputPins;
		for (UPCGPin* Pin : Node->GetInputPins())
		{
			if (Pin)
			{
				TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
				PinObj->SetStringField(TEXT("name"), Pin->Properties.Label.ToString());
				PinObj->SetNumberField(TEXT("connections"), Pin->EdgeCount());
				InputPins.Add(MakeShared<FJsonValueObject>(PinObj));
			}
		}
		NodeObj->SetArrayField(TEXT("input_pins"), InputPins);

		// Output pins
		TArray<TSharedPtr<FJsonValue>> OutputPins;
		for (UPCGPin* Pin : Node->GetOutputPins())
		{
			if (Pin)
			{
				TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
				PinObj->SetStringField(TEXT("name"), Pin->Properties.Label.ToString());
				PinObj->SetNumberField(TEXT("connections"), Pin->EdgeCount());
				OutputPins.Add(MakeShared<FJsonValueObject>(PinObj));
			}
		}
		NodeObj->SetArrayField(TEXT("output_pins"), OutputPins);

		NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
	}
	ResultObj->SetArrayField(TEXT("nodes"), NodesArray);
	ResultObj->SetNumberField(TEXT("node_count"), NodesArray.Num());

	return ResultObj;
}

TSharedPtr<FJsonObject> FPCGCommands::HandleSetPCGGraphToComponent(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString ComponentName;
	if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
	}

	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	// Find Blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Find PCG Component in Blueprint
	UPCGComponent* PCGComponent = nullptr;
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->GetVariableName().ToString() == ComponentName)
			{
				PCGComponent = Cast<UPCGComponent>(Node->ComponentTemplate);
				break;
			}
		}
	}

	if (!PCGComponent)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCGComponent not found: %s"), *ComponentName));
	}

	// Find PCG Graph
	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	// Assign graph to component
	PCGComponent->SetGraph(Graph);

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
	ResultObj->SetStringField(TEXT("component"), ComponentName);
	ResultObj->SetStringField(TEXT("graph"), GraphName);
	return ResultObj;
}

// ============================================================================
// PCG Node Creation Commands
// ============================================================================

TSharedPtr<FJsonObject> FPCGCommands::HandleAddPCGSamplerNode(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString SamplerType;
	if (!Params->TryGetStringField(TEXT("sampler_type"), SamplerType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'sampler_type' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	FVector2D Position(0.0, 0.0);
	if (const TArray<TSharedPtr<FJsonValue>>* PosArray; Params->TryGetArrayField(TEXT("node_position"), PosArray) && PosArray->Num() >= 2)
	{
		Position.X = (*PosArray)[0]->AsNumber();
		Position.Y = (*PosArray)[1]->AsNumber();
	}

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	// Map sampler type to settings class
	FString SettingsClass;
	if (SamplerType.Equals(TEXT("Surface"), ESearchCase::IgnoreCase) || SamplerType.Equals(TEXT("SurfaceSampler"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGSurfaceSamplerSettings");
	}
	else if (SamplerType.Equals(TEXT("Spline"), ESearchCase::IgnoreCase) || SamplerType.Equals(TEXT("SplineSampler"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGSplineSamplerSettings");
	}
	else if (SamplerType.Equals(TEXT("Mesh"), ESearchCase::IgnoreCase) || SamplerType.Equals(TEXT("MeshSampler"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGMeshSamplerSettings");
	}
	else if (SamplerType.Equals(TEXT("Volume"), ESearchCase::IgnoreCase) || SamplerType.Equals(TEXT("VolumeSampler"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGVolumeSamplerSettings");
	}
	else if (SamplerType.Equals(TEXT("Landscape"), ESearchCase::IgnoreCase) || SamplerType.Equals(TEXT("GetLandscapeData"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGGetLandscapeDataSettings");
	}
	else
	{
		// Try direct class name
		SettingsClass = SamplerType;
	}

	UPCGNode* NewNode = CreatePCGNode(Graph, SettingsClass, Position);
	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create sampler node: %s"), *SamplerType));
	}

	Graph->MarkPackageDirty();
	return CreatePCGNodeResponse(NewNode);
}

TSharedPtr<FJsonObject> FPCGCommands::HandleAddPCGFilterNode(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString FilterType;
	if (!Params->TryGetStringField(TEXT("filter_type"), FilterType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'filter_type' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	FVector2D Position(0.0, 0.0);
	if (const TArray<TSharedPtr<FJsonValue>>* PosArray; Params->TryGetArrayField(TEXT("node_position"), PosArray) && PosArray->Num() >= 2)
	{
		Position.X = (*PosArray)[0]->AsNumber();
		Position.Y = (*PosArray)[1]->AsNumber();
	}

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	// Map filter type to settings class
	FString SettingsClass;
	if (FilterType.Equals(TEXT("Density"), ESearchCase::IgnoreCase) || FilterType.Equals(TEXT("DensityFilter"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGDensityFilterSettings");
	}
	else if (FilterType.Equals(TEXT("Bounds"), ESearchCase::IgnoreCase) || FilterType.Equals(TEXT("BoundsFilter"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGBoundsFilterSettings");
	}
	else if (FilterType.Equals(TEXT("Point"), ESearchCase::IgnoreCase) || FilterType.Equals(TEXT("PointFilter"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGPointFilterSettings");
	}
	else if (FilterType.Equals(TEXT("SelfPruning"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGSelfPruningSettings");
	}
	else
	{
		SettingsClass = FilterType;
	}

	UPCGNode* NewNode = CreatePCGNode(Graph, SettingsClass, Position);
	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create filter node: %s"), *FilterType));
	}

	Graph->MarkPackageDirty();
	return CreatePCGNodeResponse(NewNode);
}

TSharedPtr<FJsonObject> FPCGCommands::HandleAddPCGTransformNode(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString TransformType;
	if (!Params->TryGetStringField(TEXT("transform_type"), TransformType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'transform_type' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	FVector2D Position(0.0, 0.0);
	if (const TArray<TSharedPtr<FJsonValue>>* PosArray; Params->TryGetArrayField(TEXT("node_position"), PosArray) && PosArray->Num() >= 2)
	{
		Position.X = (*PosArray)[0]->AsNumber();
		Position.Y = (*PosArray)[1]->AsNumber();
	}

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	// Map transform type to settings class
	FString SettingsClass;
	if (TransformType.Equals(TEXT("Transform"), ESearchCase::IgnoreCase) || TransformType.Equals(TEXT("TransformPoints"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGTransformPointsSettings");
	}
	else if (TransformType.Equals(TEXT("Projection"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGProjectionSettings");
	}
	else if (TransformType.Equals(TEXT("NormalToDensity"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGNormalToDensitySettings");
	}
	else if (TransformType.Equals(TEXT("BoundsModifier"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGBoundsModifierSettings");
	}
	else
	{
		SettingsClass = TransformType;
	}

	UPCGNode* NewNode = CreatePCGNode(Graph, SettingsClass, Position);
	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create transform node: %s"), *TransformType));
	}

	Graph->MarkPackageDirty();
	return CreatePCGNodeResponse(NewNode);
}

TSharedPtr<FJsonObject> FPCGCommands::HandleAddPCGSpawnerNode(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString SpawnerType;
	if (!Params->TryGetStringField(TEXT("spawner_type"), SpawnerType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'spawner_type' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	FVector2D Position(0.0, 0.0);
	if (const TArray<TSharedPtr<FJsonValue>>* PosArray; Params->TryGetArrayField(TEXT("node_position"), PosArray) && PosArray->Num() >= 2)
	{
		Position.X = (*PosArray)[0]->AsNumber();
		Position.Y = (*PosArray)[1]->AsNumber();
	}

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	// Map spawner type to settings class
	FString SettingsClass;
	if (SpawnerType.Equals(TEXT("StaticMesh"), ESearchCase::IgnoreCase) || SpawnerType.Equals(TEXT("StaticMeshSpawner"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGStaticMeshSpawnerSettings");
	}
	else if (SpawnerType.Equals(TEXT("Actor"), ESearchCase::IgnoreCase) || SpawnerType.Equals(TEXT("SpawnActor"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGSpawnActorSettings");
	}
	else if (SpawnerType.Equals(TEXT("CopyPoints"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGCopyPointsSettings");
	}
	else
	{
		SettingsClass = SpawnerType;
	}

	UPCGNode* NewNode = CreatePCGNode(Graph, SettingsClass, Position);
	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create spawner node: %s"), *SpawnerType));
	}

	Graph->MarkPackageDirty();
	return CreatePCGNodeResponse(NewNode);
}

TSharedPtr<FJsonObject> FPCGCommands::HandleAddPCGGenericNode(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString NodeClass;
	if (!Params->TryGetStringField(TEXT("node_class"), NodeClass))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'node_class' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	FVector2D Position(0.0, 0.0);
	if (const TArray<TSharedPtr<FJsonValue>>* PosArray; Params->TryGetArrayField(TEXT("node_position"), PosArray) && PosArray->Num() >= 2)
	{
		Position.X = (*PosArray)[0]->AsNumber();
		Position.Y = (*PosArray)[1]->AsNumber();
	}

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	UPCGNode* NewNode = CreatePCGNode(Graph, NodeClass, Position);
	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create PCG node: %s"), *NodeClass));
	}

	Graph->MarkPackageDirty();
	return CreatePCGNodeResponse(NewNode);
}

TSharedPtr<FJsonObject> FPCGCommands::HandleListPCGNodes(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	FString SearchQuery;
	Params->TryGetStringField(TEXT("query"), SearchQuery);

	FString SettingsFilter;
	Params->TryGetStringField(TEXT("settings_class"), SettingsFilter);

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	TArray<TSharedPtr<FJsonValue>> ResultsArray;
	for (UPCGNode* Node : Graph->GetNodes())
	{
		if (!Node)
		{
			continue;
		}

		UPCGSettings* Settings = Node->GetSettings();
		FString SettingsClassName = Settings ? Settings->GetClass()->GetName() : TEXT("");
		FString NodeId = Node->GetFName().ToString();

		bool bMatches = true;

		if (!SearchQuery.IsEmpty())
		{
			bMatches = NodeId.Contains(SearchQuery, ESearchCase::IgnoreCase) ||
					   SettingsClassName.Contains(SearchQuery, ESearchCase::IgnoreCase);
		}

		if (bMatches && !SettingsFilter.IsEmpty())
		{
			bMatches = SettingsClassName.Contains(SettingsFilter, ESearchCase::IgnoreCase);
		}

		if (bMatches)
		{
			TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
			NodeObj->SetStringField(TEXT("node_id"), NodeId);
			NodeObj->SetStringField(TEXT("settings_class"), SettingsClassName);
			NodeObj->SetNumberField(TEXT("position_x"), Node->PositionX);
			NodeObj->SetNumberField(TEXT("position_y"), Node->PositionY);
			ResultsArray.Add(MakeShared<FJsonValueObject>(NodeObj));
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetArrayField(TEXT("nodes"), ResultsArray);
	ResultObj->SetNumberField(TEXT("count"), ResultsArray.Num());
	return ResultObj;
}

TSharedPtr<FJsonObject> FPCGCommands::HandleAddPCGAttributeNode(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString AttributeType;
	if (!Params->TryGetStringField(TEXT("attribute_type"), AttributeType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'attribute_type' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	FVector2D Position(0.0, 0.0);
	if (const TArray<TSharedPtr<FJsonValue>>* PosArray; Params->TryGetArrayField(TEXT("node_position"), PosArray) && PosArray->Num() >= 2)
	{
		Position.X = (*PosArray)[0]->AsNumber();
		Position.Y = (*PosArray)[1]->AsNumber();
	}

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	FString SettingsClass;
	if (AttributeType.Equals(TEXT("CreateAttribute"), ESearchCase::IgnoreCase) || AttributeType.Equals(TEXT("Create"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGCreateAttributeSettings");
	}
	else if (AttributeType.Equals(TEXT("DeleteAttribute"), ESearchCase::IgnoreCase) || AttributeType.Equals(TEXT("Delete"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGDeleteAttributeSettings");
	}
	else if (AttributeType.Equals(TEXT("CopyAttribute"), ESearchCase::IgnoreCase) || AttributeType.Equals(TEXT("Copy"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGCopyAttributeSettings");
	}
	else if (AttributeType.Equals(TEXT("RenameAttribute"), ESearchCase::IgnoreCase) || AttributeType.Equals(TEXT("Rename"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGRenameAttributeSettings");
	}
	else if (AttributeType.Equals(TEXT("Metadata"), ESearchCase::IgnoreCase) || AttributeType.Equals(TEXT("MetadataBreakdown"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGMetadataBreakdownSettings");
	}
	else if (AttributeType.Equals(TEXT("AttributeNoise"), ESearchCase::IgnoreCase) || AttributeType.Equals(TEXT("Noise"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGAttributeNoiseSettings");
	}
	else if (AttributeType.Equals(TEXT("PropertyToParams"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGPropertyToParamDataSettings");
	}
	else
	{
		SettingsClass = AttributeType;
	}

	UPCGNode* NewNode = CreatePCGNode(Graph, SettingsClass, Position);
	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create attribute node: %s"), *AttributeType));
	}

	Graph->MarkPackageDirty();
	return CreatePCGNodeResponse(NewNode);
}

TSharedPtr<FJsonObject> FPCGCommands::HandleAddPCGFlowControlNode(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString FlowType;
	if (!Params->TryGetStringField(TEXT("flow_type"), FlowType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'flow_type' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	FVector2D Position(0.0, 0.0);
	if (const TArray<TSharedPtr<FJsonValue>>* PosArray; Params->TryGetArrayField(TEXT("node_position"), PosArray) && PosArray->Num() >= 2)
	{
		Position.X = (*PosArray)[0]->AsNumber();
		Position.Y = (*PosArray)[1]->AsNumber();
	}

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	FString SettingsClass;
	if (FlowType.Equals(TEXT("Branch"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGBranchSettings");
	}
	else if (FlowType.Equals(TEXT("Collapse"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGCollapseSettings");
	}
	else if (FlowType.Equals(TEXT("Merge"), ESearchCase::IgnoreCase) || FlowType.Equals(TEXT("Union"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGUnionSettings");
	}
	else if (FlowType.Equals(TEXT("Difference"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGDifferenceSettings");
	}
	else if (FlowType.Equals(TEXT("Intersection"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGIntersectionSettings");
	}
	else if (FlowType.Equals(TEXT("Subgraph"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGSubgraphSettings");
	}
	else if (FlowType.Equals(TEXT("Loop"), ESearchCase::IgnoreCase))
	{
		SettingsClass = TEXT("PCGLoopSettings");
	}
	else
	{
		SettingsClass = FlowType;
	}

	UPCGNode* NewNode = CreatePCGNode(Graph, SettingsClass, Position);
	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create flow control node: %s"), *FlowType));
	}

	Graph->MarkPackageDirty();
	return CreatePCGNodeResponse(NewNode);
}

// ============================================================================
// PCG Node Connection Commands
// ============================================================================

TSharedPtr<FJsonObject> FPCGCommands::HandleConnectPCGNodes(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString SourceNodeId;
	if (!Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'source_node_id' parameter"));
	}

	FString TargetNodeId;
	if (!Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'target_node_id' parameter"));
	}

	FString SourcePinName;
	Params->TryGetStringField(TEXT("source_pin"), SourcePinName);

	FString TargetPinName;
	Params->TryGetStringField(TEXT("target_pin"), TargetPinName);

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	// Find nodes
	UPCGNode* SourceNode = nullptr;
	UPCGNode* TargetNode = nullptr;

	for (UPCGNode* Node : Graph->GetNodes())
	{
		if (Node->GetFName().ToString() == SourceNodeId)
		{
			SourceNode = Node;
		}
		if (Node->GetFName().ToString() == TargetNodeId)
		{
			TargetNode = Node;
		}
	}

	if (!SourceNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Source node not found: %s"), *SourceNodeId));
	}
	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Target node not found: %s"), *TargetNodeId));
	}

	// Find pins
	UPCGPin* SourcePin = nullptr;
	UPCGPin* TargetPin = nullptr;

	// Find source pin (output)
	for (UPCGPin* Pin : SourceNode->GetOutputPins())
	{
		if (SourcePinName.IsEmpty() || Pin->Properties.Label.ToString() == SourcePinName)
		{
			SourcePin = Pin;
			break;
		}
	}

	// Find target pin (input)
	for (UPCGPin* Pin : TargetNode->GetInputPins())
	{
		if (TargetPinName.IsEmpty() || Pin->Properties.Label.ToString() == TargetPinName)
		{
			TargetPin = Pin;
			break;
		}
	}

	if (!SourcePin)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Source pin not found"));
	}
	if (!TargetPin)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Target pin not found"));
	}

	// Connect
	bool bConnected = SourcePin->AddEdgeTo(TargetPin);
	if (!bConnected)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to connect pins"));
	}

	Graph->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("source_node"), SourceNodeId);
	ResultObj->SetStringField(TEXT("target_node"), TargetNodeId);
	ResultObj->SetStringField(TEXT("source_pin"), SourcePin->Properties.Label.ToString());
	ResultObj->SetStringField(TEXT("target_pin"), TargetPin->Properties.Label.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FPCGCommands::HandleDisconnectPCGNodes(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	FString PinName;
	if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	// Find node
	UPCGNode* Node = nullptr;
	for (UPCGNode* N : Graph->GetNodes())
	{
		if (N->GetFName().ToString() == NodeId)
		{
			Node = N;
			break;
		}
	}

	if (!Node)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	// Find and disconnect pin
	UPCGPin* Pin = nullptr;
	for (UPCGPin* P : Node->GetInputPins())
	{
		if (P->Properties.Label.ToString() == PinName)
		{
			Pin = P;
			break;
		}
	}
	if (!Pin)
	{
		for (UPCGPin* P : Node->GetOutputPins())
		{
			if (P->Properties.Label.ToString() == PinName)
			{
				Pin = P;
				break;
			}
		}
	}

	if (!Pin)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Pin not found: %s"), *PinName));
	}

	Pin->BreakAllEdges();
	Graph->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("node_id"), NodeId);
	ResultObj->SetStringField(TEXT("pin_name"), PinName);
	return ResultObj;
}

// ============================================================================
// PCG Node Deletion
// ============================================================================

TSharedPtr<FJsonObject> FPCGCommands::HandleDeletePCGNode(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	FString GraphPath = TEXT("/Game/PCG/");
	Params->TryGetStringField(TEXT("graph_path"), GraphPath);

	UPCGGraph* Graph = FindPCGGraph(GraphName, GraphPath);
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("PCG Graph not found: %s"), *GraphName));
	}

	// Find node
	UPCGNode* NodeToDelete = nullptr;
	for (UPCGNode* Node : Graph->GetNodes())
	{
		if (Node->GetFName().ToString() == NodeId)
		{
			NodeToDelete = Node;
			break;
		}
	}

	if (!NodeToDelete)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	Graph->RemoveNode(NodeToDelete);
	Graph->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("deleted_node_id"), NodeId);
	return ResultObj;
}
