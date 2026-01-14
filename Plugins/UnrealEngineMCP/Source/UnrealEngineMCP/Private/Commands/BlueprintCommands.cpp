#include "Commands/BlueprintCommands.h"
#include "Commands/CommonUtils.h"
#include "Dom/JsonObject.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/InheritableComponentHandler.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
// Blueprint node graph includes (merged from BlueprintNodeCommands)
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_FunctionEntry.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "EdGraphNode_Comment.h"
#include "Misc/App.h"
// GAS (Gameplay Ability System) includes
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectExecutionCalculation.h"
#include "AttributeSet.h"
#include "UObject/SavePackage.h"
// Additional Blueprint nodes
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_VariableSet.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_GetDataTableRow.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_SwitchInteger.h"
#include "K2Node_SwitchString.h"
#include "K2Node_Select.h"
#include "K2Node_AddComponent.h"
#include "K2Node_GetClassDefaults.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_MakeArray.h"
#include "K2Node_CommutativeAssociativeBinaryOperator.h"
#include "Engine/DataTable.h"

FBlueprintCommands::FBlueprintCommands()
{
}

FBlueprintCommands::~FBlueprintCommands()
{
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("create_blueprint"))
	{
		return HandleCreateBlueprint(Params);
	}
	else if (CommandType == TEXT("add_component_to_blueprint"))
	{
		return HandleAddComponentToBlueprint(Params);
	}
	else if (CommandType == TEXT("set_component_property"))
	{
		return HandleSetComponentProperty(Params);
	}
	else if (CommandType == TEXT("set_physics_properties"))
	{
		return HandleSetPhysicsProperties(Params);
	}
	else if (CommandType == TEXT("compile_blueprint"))
	{
		return HandleCompileBlueprint(Params);
	}
	else if (CommandType == TEXT("set_mesh_material_color"))
	{
		return HandleSetMeshMaterialColor(Params);
	}
	// Blueprint node graph commands (merged from BlueprintNodeCommands)
	else if (CommandType == TEXT("connect_blueprint_nodes"))
	{
		return HandleConnectBlueprintNodes(Params);
	}
	else if (CommandType == TEXT("add_component_getter_node"))
	{
		return HandleAddComponentGetterNode(Params);
	}
	else if (CommandType == TEXT("add_blueprint_event_node"))
	{
		return HandleAddBlueprintEvent(Params);
	}
	else if (CommandType == TEXT("add_custom_event_node"))
	{
		return HandleAddCustomEventNode(Params);
	}
	else if (CommandType == TEXT("add_blueprint_function_node"))
	{
		return HandleAddBlueprintFunctionCall(Params);
	}
	else if (CommandType == TEXT("add_blueprint_variable"))
	{
		return HandleAddBlueprintVariable(Params);
	}
	else if (CommandType == TEXT("add_blueprint_input_action_node"))
	{
		return HandleAddBlueprintInputActionNode(Params);
	}
	else if (CommandType == TEXT("add_blueprint_self_reference"))
	{
		return HandleAddBlueprintSelfReference(Params);
	}
	else if (CommandType == TEXT("list_blueprint_nodes"))
	{
		return HandleListBlueprintNodes(Params);
	}
	// Material commands
	else if (CommandType == TEXT("apply_material_to_blueprint"))
	{
		return HandleApplyMaterialToBlueprint(Params);
	}
	else if (CommandType == TEXT("get_blueprint_material_info"))
	{
		return HandleGetBlueprintMaterialInfo(Params);
	}
	// Organization commands
	else if (CommandType == TEXT("add_comment_box"))
	{
		return HandleAddCommentBox(Params);
	}
	else if (CommandType == TEXT("analyze_blueprint"))
	{
		return HandleAnalyzeBlueprint(Params);
	}
	// GAS (Gameplay Ability System) commands
	else if (CommandType == TEXT("create_gameplay_effect"))
	{
		return HandleCreateGameplayEffect(Params);
	}
	else if (CommandType == TEXT("create_gameplay_ability"))
	{
		return HandleCreateGameplayAbility(Params);
	}
	// GAS AttributeSet commands
	else if (CommandType == TEXT("list_attribute_sets"))
	{
		return HandleListAttributeSets(Params);
	}
	else if (CommandType == TEXT("get_attribute_set_info"))
	{
		return HandleGetAttributeSetInfo(Params);
	}
	// Tier 1: Core Blueprint node tools
	else if (CommandType == TEXT("add_blueprint_flow_control_node"))
	{
		return HandleAddBlueprintFlowControlNode(Params);
	}
	else if (CommandType == TEXT("set_pin_default_value"))
	{
		return HandleSetPinDefaultValue(Params);
	}
	else if (CommandType == TEXT("get_pin_value"))
	{
		return HandleGetPinValue(Params);
	}
	else if (CommandType == TEXT("add_blueprint_variable_node"))
	{
		return HandleAddBlueprintVariableNode(Params);
	}
	else if (CommandType == TEXT("search_functions"))
	{
		return HandleSearchFunctions(Params);
	}
	else if (CommandType == TEXT("get_class_functions"))
	{
		return HandleGetClassFunctions(Params);
	}
	else if (CommandType == TEXT("get_class_properties"))
	{
		return HandleGetClassProperties(Params);
	}
	else if (CommandType == TEXT("get_blueprint_variables"))
	{
		return HandleGetBlueprintVariables(Params);
	}
	else if (CommandType == TEXT("add_property_get_set_node"))
	{
		return HandleAddPropertyGetSetNode(Params);
	}
	else if (CommandType == TEXT("add_function_override"))
	{
		return HandleAddFunctionOverride(Params);
	}
	else if (CommandType == TEXT("add_ability_task_node"))
	{
		return HandleAddAbilityTaskNode(Params);
	}
	// Generic node tools
	else if (CommandType == TEXT("add_blueprint_generic_node"))
	{
		return HandleAddGenericNode(Params);
	}
	else if (CommandType == TEXT("set_node_property"))
	{
		return HandleSetNodeProperty(Params);
	}
	else if (CommandType == TEXT("connect_nodes"))
	{
		return HandleConnectNodes(Params);
	}
	else if (CommandType == TEXT("list_graphs"))
	{
		return HandleListGraphs(Params);
	}
	else if (CommandType == TEXT("create_child_blueprint"))
	{
		return HandleCreateChildBlueprint(Params);
	}
	// Declarative graph builder
	else if (CommandType == TEXT("build_ability_graph"))
	{
		return HandleBuildAbilityGraph(Params);
	}
	// Deletion commands
	else if (CommandType == TEXT("delete_blueprint_node"))
	{
		return HandleDeleteBlueprintNode(Params);
	}
	else if (CommandType == TEXT("delete_blueprint_variable"))
	{
		return HandleDeleteBlueprintVariable(Params);
	}
	else if (CommandType == TEXT("delete_component_from_blueprint"))
	{
		return HandleDeleteComponentFromBlueprint(Params);
	}
	else if (CommandType == TEXT("disconnect_blueprint_nodes"))
	{
		return HandleDisconnectBlueprintNodes(Params);
	}
	// Dynamic pin management
	else if (CommandType == TEXT("add_pin"))
	{
		return HandleAddPin(Params);
	}
	else if (CommandType == TEXT("delete_pin"))
	{
		return HandleDeletePin(Params);
	}

	return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	// Get optional path parameter (default: /Game/Blueprints/)
	FString PackagePath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("path"), PackagePath);

	// Ensure path ends with /
	if (!PackagePath.EndsWith(TEXT("/")))
	{
		PackagePath += TEXT("/");
	}

	// Check if blueprint already exists - return existing if found
	FString AssetName = BlueprintName;
	FString FullAssetPath = PackagePath + AssetName;
	if (UEditorAssetLibrary::DoesAssetExist(FullAssetPath))
	{
		UBlueprint* ExistingBlueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullAssetPath));
		if (ExistingBlueprint)
		{
			TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
			ResultObj->SetStringField(TEXT("name"), AssetName);
			ResultObj->SetStringField(TEXT("path"), FullAssetPath);
			ResultObj->SetBoolField(TEXT("success"), true);
			ResultObj->SetBoolField(TEXT("already_exists"), true);
			return ResultObj;
		}
	}

	// Create the blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	
	// Handle parent class
	FString ParentClass;
	Params->TryGetStringField(TEXT("parent_class"), ParentClass);
	
	// Default to Actor if no parent class specified
	UClass* SelectedParentClass = AActor::StaticClass();
	
	// Try to find the specified parent class
	if (!ParentClass.IsEmpty())
	{
		UClass* FoundClass = FCommonUtils::FindClassByName(ParentClass);
		if (FoundClass && FoundClass->IsChildOf(AActor::StaticClass()))
		{
			SelectedParentClass = FoundClass;
		}
	}
	
	Factory->ParentClass = SelectedParentClass;

	// Create the blueprint
	UPackage* Package = CreatePackage(*(PackagePath + AssetName));
	UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(UBlueprint::StaticClass(), Package, *AssetName, RF_Standalone | RF_Public, nullptr, GWarn));

	if (NewBlueprint)
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(NewBlueprint);

		// Mark the package dirty
		Package->MarkPackageDirty();

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("name"), AssetName);
		ResultObj->SetStringField(TEXT("path"), PackagePath + AssetName);
		ResultObj->SetBoolField(TEXT("success"), true);
		return ResultObj;
	}

	return FCommonUtils::CreateErrorResponse(TEXT("Failed to create blueprint"));
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString ComponentType;
	if (!Params->TryGetStringField(TEXT("component_type"), ComponentType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'component_type' parameter"));
	}

	FString ComponentName;
	if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
	}

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Create the component
	UClass* ComponentClass = nullptr;

	// Try to find the class with exact name first
	ComponentClass = FindFirstObject<UClass>(*ComponentType, EFindFirstObjectOptions::ExactClass);

	// If not found, try with "Component" suffix
	if (!ComponentClass && !ComponentType.EndsWith(TEXT("Component")))
	{
		FString ComponentTypeWithSuffix = ComponentType + TEXT("Component");
		ComponentClass = FindFirstObject<UClass>(*ComponentTypeWithSuffix, EFindFirstObjectOptions::ExactClass);
	}
	
	// Verify that the class is a valid component type
	if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown component type: %s"), *ComponentType));
	}

	// Add the component to the blueprint
	USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, *ComponentName);
	if (NewNode)
	{
		// Set transform if provided
		USceneComponent* SceneComponent = Cast<USceneComponent>(NewNode->ComponentTemplate);
		if (SceneComponent)
		{
			if (Params->HasField(TEXT("location")))
			{
				SceneComponent->SetRelativeLocation(FCommonUtils::GetVectorFromJson(Params, TEXT("location")));
			}
			if (Params->HasField(TEXT("rotation")))
			{
				SceneComponent->SetRelativeRotation(FCommonUtils::GetRotatorFromJson(Params, TEXT("rotation")));
			}
			if (Params->HasField(TEXT("scale")))
			{
				SceneComponent->SetRelativeScale3D(FCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
			}
		}

		// Add to root if no parent specified
		Blueprint->SimpleConstructionScript->AddNode(NewNode);

		// Compile the blueprint
		FKismetEditorUtilities::CompileBlueprint(Blueprint);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("component_name"), ComponentName);
		ResultObj->SetStringField(TEXT("component_type"), ComponentType);
		ResultObj->SetBoolField(TEXT("success"), true);
		return ResultObj;
	}

	return FCommonUtils::CreateErrorResponse(TEXT("Failed to add component to blueprint"));
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleSetComponentProperty(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
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

	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	UObject* ComponentTemplate = nullptr;
	FString ComponentSource;

	// Search in SCS (this Blueprint's own components)
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->GetVariableName().ToString() == ComponentName)
			{
				ComponentTemplate = Node->ComponentTemplate;
				ComponentSource = TEXT("scs");
				break;
			}
		}
	}

	// Search in CDO (inherited components from parent class)
	if (!ComponentTemplate && Blueprint->GeneratedClass)
	{
		UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
		if (CDO)
		{
			for (TFieldIterator<FObjectProperty> PropIt(Blueprint->GeneratedClass); PropIt; ++PropIt)
			{
				FObjectProperty* ObjProp = *PropIt;
				if (ObjProp->PropertyClass->IsChildOf(UActorComponent::StaticClass()))
				{
					UObject* CompObj = ObjProp->GetObjectPropertyValue_InContainer(CDO);
					if (CompObj && (ObjProp->GetName() == ComponentName || CompObj->GetName() == ComponentName))
					{
						ComponentTemplate = CompObj;
						ComponentSource = TEXT("cdo");
						break;
					}
				}
			}
		}
	}

	// Search in InheritableComponentHandler (overridden inherited components)
	if (!ComponentTemplate && Blueprint->InheritableComponentHandler)
	{
		for (auto It = Blueprint->InheritableComponentHandler->CreateRecordIterator(); It; ++It)
		{
			UActorComponent* CompTemplate = It->ComponentTemplate;
			const FComponentKey& Key = It->ComponentKey;
			if (CompTemplate && (CompTemplate->GetName() == ComponentName || Key.GetSCSVariableName().ToString() == ComponentName))
			{
				ComponentTemplate = CompTemplate;
				ComponentSource = TEXT("inherited_override");
				break;
			}
		}
	}

	if (!ComponentTemplate)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	if (!Params->HasField(TEXT("value")) && !Params->HasField(TEXT("property_value")))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	TSharedPtr<FJsonValue> JsonValue = Params->HasField(TEXT("value"))
		? Params->Values.FindRef(TEXT("value"))
		: Params->Values.FindRef(TEXT("property_value"));

	FString ErrorMessage;
	if (FCommonUtils::SetObjectProperty(ComponentTemplate, PropertyName, JsonValue, ErrorMessage))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("component"), ComponentName);
		ResultObj->SetStringField(TEXT("property"), PropertyName);
		ResultObj->SetStringField(TEXT("source"), ComponentSource);
		ResultObj->SetBoolField(TEXT("success"), true);
		return ResultObj;
	}

	return FCommonUtils::CreateErrorResponse(ErrorMessage);
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
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

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Find the component
	USCS_Node* ComponentNode = nullptr;
	for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
	{
		if (Node && Node->GetVariableName().ToString() == ComponentName)
		{
			ComponentNode = Node;
			break;
		}
	}

	if (!ComponentNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
	if (!PrimComponent)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
	}

	// Set physics properties
	if (Params->HasField(TEXT("simulate_physics")))
	{
		PrimComponent->SetSimulatePhysics(Params->GetBoolField(TEXT("simulate_physics")));
	}

	if (Params->HasField(TEXT("mass")))
	{
		PrimComponent->SetMassOverrideInKg(NAME_None, Params->GetNumberField(TEXT("mass")));
	}

	if (Params->HasField(TEXT("enable_gravity")))
	{
		PrimComponent->SetEnableGravity(Params->GetBoolField(TEXT("enable_gravity")));
	}

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("component"), ComponentName);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	bool bValidateOnly = false;
	Params->TryGetBoolField(TEXT("validate_only"), bValidateOnly);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Validate graphs before compile
	TArray<TSharedPtr<FJsonValue>> Issues = FCommonUtils::ValidateBlueprintGraphs(Blueprint);

	if (Issues.Num() > 0)
	{
		FInstructionContext Ctx;
		Ctx.Type = TEXT("validation_failed");
		Ctx.Message = FString::Printf(TEXT("%d issues found. Fix before compiling."), Issues.Num());
		Ctx.ActionHint = TEXT("Connect missing required pins listed in validation_issues.");

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("success"), false);
		ResultObj->SetStringField(TEXT("error"), McpMessages::ValidationFailed);
		ResultObj->SetArrayField(TEXT("validation_issues"), Issues);
		ResultObj->SetObjectField(TEXT("instruction"), Ctx.ToJson());
		return ResultObj;
	}

	if (bValidateOnly)
	{
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("success"), true);
		ResultObj->SetStringField(TEXT("message"), TEXT("Validation passed. Ready to compile."));
		return ResultObj;
	}

	FCompilerResultsLog ResultsLog;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &ResultsLog);

	// Check compile result
	bool bHasErrors = Blueprint->Status == BS_Error;
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
	ResultObj->SetBoolField(TEXT("success"), !bHasErrors);

	if (bHasErrors)
	{
		TArray<TSharedPtr<FJsonValue>> CompileErrors;
		for (const TSharedRef<FTokenizedMessage>& Msg : ResultsLog.Messages)
		{
			TSharedPtr<FJsonObject> ErrObj = MakeShared<FJsonObject>();
			ErrObj->SetStringField(TEXT("message"), Msg->ToText().ToString());
			ErrObj->SetStringField(TEXT("severity"),
				Msg->GetSeverity() == EMessageSeverity::Error ? TEXT("error") : TEXT("warning"));
			CompileErrors.Add(MakeShared<FJsonValueObject>(ErrObj));
		}
		ResultObj->SetArrayField(TEXT("compile_errors"), CompileErrors);
	}

	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleSetMeshMaterialColor(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
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

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Find the component
	USCS_Node* ComponentNode = nullptr;
	for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
	{
		if (Node && Node->GetVariableName().ToString() == ComponentName)
		{
			ComponentNode = Node;
			break;
		}
	}

	if (!ComponentNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	// Cast to PrimitiveComponent
	UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
	if (!PrimComponent)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
	}

	// Get color parameter [R, G, B, A]
	TArray<float> ColorArray;
	const TArray<TSharedPtr<FJsonValue>>* ColorJsonArray;
	if (!Params->TryGetArrayField(TEXT("color"), ColorJsonArray) || ColorJsonArray->Num() != 4)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("'color' must be an array of 4 float values [R, G, B, A]"));
	}

	for (const TSharedPtr<FJsonValue>& Value : *ColorJsonArray)
	{
		ColorArray.Add(FMath::Clamp((float)Value->AsNumber(), 0.0f, 1.0f));
	}

	FLinearColor Color(ColorArray[0], ColorArray[1], ColorArray[2], ColorArray[3]);

	// Get material slot index (optional, default 0)
	int32 MaterialSlot = 0;
	if (Params->HasField(TEXT("material_slot")))
	{
		MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
	}

	// Get parameter name (optional, default "BaseColor")
	FString ParameterName = TEXT("BaseColor");
	Params->TryGetStringField(TEXT("parameter_name"), ParameterName);

	// Get or load material
	UMaterialInterface* Material = nullptr;

	FString MaterialPath;
	if (Params->TryGetStringField(TEXT("material_path"), MaterialPath))
	{
		Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
		if (!Material)
		{
			return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
		}
	}
	else
	{
		// Use existing material on the component
		Material = PrimComponent->GetMaterial(MaterialSlot);
		if (!Material)
		{
			// Fall back to default material
			Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial")));
			if (!Material)
			{
				return FCommonUtils::CreateErrorResponse(TEXT("No material found and failed to load default material"));
			}
		}
	}

	// Create a dynamic material instance
	UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(Material, PrimComponent);
	if (!DynMaterial)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create dynamic material instance"));
	}

	// Set the color parameter
	DynMaterial->SetVectorParameterValue(*ParameterName, Color);

	// Apply the material to the component
	PrimComponent->SetMaterial(MaterialSlot, DynMaterial);

	// Mark the blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	// Log success
	UE_LOG(LogTemp, Display, TEXT("FBlueprintCommands::HandleSetMeshMaterialColor: Set material color on component %s: R=%f, G=%f, B=%f, A=%f"),
		*ComponentName, Color.R, Color.G, Color.B, Color.A);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("component"), ComponentName);
	ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
	ResultObj->SetStringField(TEXT("parameter_name"), ParameterName);

	TArray<TSharedPtr<FJsonValue>> ColorResultArray;
	ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.R));
	ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.G));
	ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.B));
	ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.A));
	ResultObj->SetArrayField(TEXT("color"), ColorResultArray);

	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

// ============================================================================
// Blueprint Node Graph Commands (merged from BlueprintNodeCommands)
// ============================================================================

TSharedPtr<FJsonObject> FBlueprintCommands::HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString SourcePinName;
	if (!Params->TryGetStringField(TEXT("source_pin"), SourcePinName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'source_pin' parameter"));
	}

	FString TargetPinName;
	if (!Params->TryGetStringField(TEXT("target_pin"), TargetPinName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'target_pin' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Get target graph (default: EventGraph)
	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	UEdGraph* TargetGraph = nullptr;
	if (GraphName.IsEmpty())
	{
		TargetGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	}
	else
	{
		TargetGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	}

	if (!TargetGraph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	UEdGraphNode* SourceNode = nullptr;
	UEdGraphNode* TargetNode = nullptr;

	// Lambda for node search by criteria
	auto FindNodeByCriteria = [&TargetGraph](const TSharedPtr<FJsonObject>& SearchParams) -> UEdGraphNode*
	{
		if (!SearchParams.IsValid()) return nullptr;

		FString NodeTitle;
		SearchParams->TryGetStringField(TEXT("node_title"), NodeTitle);

		FString NodeClass;
		SearchParams->TryGetStringField(TEXT("node_class"), NodeClass);

		FString EventName;
		SearchParams->TryGetStringField(TEXT("event_name"), EventName);

		bool bNewest = false;
		SearchParams->TryGetBoolField(TEXT("newest"), bNewest);

		bool bHasUnconnectedExecOut = false;
		SearchParams->TryGetBoolField(TEXT("has_unconnected_exec_out"), bHasUnconnectedExecOut);

		TArray<UEdGraphNode*> Candidates;

		for (UEdGraphNode* Node : TargetGraph->Nodes)
		{
			if (!Node) continue;

			bool bMatches = true;

			if (!NodeTitle.IsEmpty())
			{
				FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
				if (!Title.Contains(NodeTitle, ESearchCase::IgnoreCase))
				{
					bMatches = false;
				}
			}

			if (bMatches && !NodeClass.IsEmpty())
			{
				if (Node->GetClass()->GetName() != NodeClass)
				{
					bMatches = false;
				}
			}

			if (bMatches && !EventName.IsEmpty())
			{
				UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
				if (!EventNode || EventNode->EventReference.GetMemberName() != FName(*EventName))
				{
					bMatches = false;
				}
			}

			if (bMatches && bHasUnconnectedExecOut)
			{
				bool bFound = false;
				for (UEdGraphPin* Pin : Node->Pins)
				{
					if (Pin && Pin->Direction == EGPD_Output &&
						Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec &&
						Pin->LinkedTo.Num() == 0)
					{
						bFound = true;
						break;
					}
				}
				if (!bFound) bMatches = false;
			}

			if (bMatches)
			{
				Candidates.Add(Node);
			}
		}

		if (Candidates.Num() == 0) return nullptr;

		if (bNewest)
		{
			Candidates.Sort([](const UEdGraphNode& A, const UEdGraphNode& B) {
				return A.NodePosX > B.NodePosX;
			});
		}

		return Candidates[0];
	};

	FString SourceNodeId;
	FString TargetNodeId;

	// Find source node by GUID or search criteria
	if (Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId))
	{
		for (UEdGraphNode* Node : TargetGraph->Nodes)
		{
			if (Node->NodeGuid.ToString() == SourceNodeId)
			{
				SourceNode = Node;
				break;
			}
		}
	}
	else if (Params->HasField(TEXT("source_search")))
	{
		const TSharedPtr<FJsonObject>* SourceSearchPtr;
		if (Params->TryGetObjectField(TEXT("source_search"), SourceSearchPtr))
		{
			SourceNode = FindNodeByCriteria(*SourceSearchPtr);
			if (SourceNode)
			{
				SourceNodeId = SourceNode->NodeGuid.ToString();
			}
		}
	}

	// Find target node by GUID or search criteria
	if (Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId))
	{
		for (UEdGraphNode* Node : TargetGraph->Nodes)
		{
			if (Node->NodeGuid.ToString() == TargetNodeId)
			{
				TargetNode = Node;
				break;
			}
		}
	}
	else if (Params->HasField(TEXT("target_search")))
	{
		const TSharedPtr<FJsonObject>* TargetSearchPtr;
		if (Params->TryGetObjectField(TEXT("target_search"), TargetSearchPtr))
		{
			TargetNode = FindNodeByCriteria(*TargetSearchPtr);
			if (TargetNode)
			{
				TargetNodeId = TargetNode->NodeGuid.ToString();
			}
		}
	}

	if (!SourceNode)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Source node not found. Provide 'source_node_id' (GUID) or 'source_search' with node_title, event_name, node_class, newest, or has_unconnected_exec_out."));
	}

	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Target node not found. Provide 'target_node_id' (GUID) or 'target_search' with node_title, event_name, node_class, newest, or has_unconnected_exec_out."));
	}

	// Find pins and attempt connection
	UEdGraphPin* SourcePin = FCommonUtils::FindPin(SourceNode, SourcePinName, EGPD_Output);
	UEdGraphPin* TargetPin = FCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_Input);

	// Build instruction context for pin not found errors
	auto BuildPinNotFoundInstruction = [](UEdGraphNode* Node, const FString& RequestedPin, EEdGraphPinDirection Dir) -> FInstructionContext
	{
		FInstructionContext Ctx;
		Ctx.Type = TEXT("pin_not_found");
		Ctx.Message = FString::Printf(TEXT("Pin '%s' not found on node '%s'."), *RequestedPin, *Node->GetNodeTitle(ENodeTitleType::ListView).ToString());

		TArray<FString> AvailablePins;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && !Pin->bHidden && Pin->Direction == Dir)
			{
				AvailablePins.Add(Pin->PinName.ToString());
			}
		}
		Ctx.Suggestions = FCommonUtils::FindSimilarNames(RequestedPin, AvailablePins);
		if (Ctx.Suggestions.Num() == 0)
		{
			Ctx.Suggestions = AvailablePins;
		}
		Ctx.ActionHint = TEXT("Use one of the suggested pin names.");
		return Ctx;
	};

	if (!SourcePin)
	{
		FInstructionContext Ctx = BuildPinNotFoundInstruction(SourceNode, SourcePinName, EGPD_Output);
		return FCommonUtils::CreateErrorWithInstruction(Ctx.Message, Ctx);
	}

	if (!TargetPin)
	{
		FInstructionContext Ctx = BuildPinNotFoundInstruction(TargetNode, TargetPinName, EGPD_Input);
		return FCommonUtils::CreateErrorWithInstruction(Ctx.Message, Ctx);
	}

	// Make connection
	SourcePin->MakeLinkTo(TargetPin);

	// Check if either pin needs type propagation (Wildcard, Class, Object, etc.)
	auto NeedsReconstruct = [](UEdGraphPin* Pin) -> bool
	{
		if (!Pin) return false;
		FName PinCategory = Pin->PinType.PinCategory;
		return (
			PinCategory == UEdGraphSchema_K2::PC_Class ||
			PinCategory == UEdGraphSchema_K2::PC_Object ||
			PinCategory == UEdGraphSchema_K2::PC_Interface ||
			PinCategory == UEdGraphSchema_K2::PC_SoftClass ||
			PinCategory == UEdGraphSchema_K2::PC_SoftObject ||
			PinCategory == UEdGraphSchema_K2::PC_Wildcard ||
			Pin->PinName == TEXT("Class") ||
			Pin->PinName == TEXT("Template")
		);
	};

	// Reconstruct source node if needed (e.g., MakeArray with Wildcard pins)
	if (NeedsReconstruct(SourcePin))
	{
		SourceNode->PinConnectionListChanged(SourcePin);
		SourceNode->ReconstructNode();
	}

	// Reconstruct target node if needed
	if (NeedsReconstruct(TargetPin))
	{
		TargetNode->PinConnectionListChanged(TargetPin);
		TargetNode->ReconstructNode();
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	// Return with instruction context (auto-detect missing pins on target node)
	TSharedPtr<FJsonObject> ResultObj = FCommonUtils::CreateNodeResponseWithContext(TargetNode, FInstructionContext());
	ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
	ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
	ResultObj->SetStringField(TEXT("source_node_title"), SourceNode->GetNodeTitle(ENodeTitleType::ListView).ToString());
	ResultObj->SetStringField(TEXT("target_node_title"), TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString());
	ResultObj->SetArrayField(TEXT("source_node_pins"), FCommonUtils::NodePinsToJson(SourceNode));
	ResultObj->SetArrayField(TEXT("target_node_pins"), FCommonUtils::NodePinsToJson(TargetNode));
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddComponentGetterNode(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
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

	// Get position parameters (optional)
	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Get the event graph
	UEdGraph* EventGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
	}

	// Create the variable get node
	UK2Node_VariableGet* GetComponentNode = NewObject<UK2Node_VariableGet>(EventGraph);
	if (!GetComponentNode)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create get component node"));
	}

	// Set up the variable reference
	FMemberReference& VarRef = GetComponentNode->VariableReference;
	VarRef.SetSelfMember(FName(*ComponentName));

	// Set node position
	GetComponentNode->NodePosX = NodePosition.X;
	GetComponentNode->NodePosY = NodePosition.Y;

	// Add to graph
	EventGraph->AddNode(GetComponentNode);
	GetComponentNode->CreateNewGuid();
	GetComponentNode->PostPlacedNewNode();
	GetComponentNode->AllocateDefaultPins();

	// Reconstruct node
	GetComponentNode->ReconstructNode();

	// Mark the blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), GetComponentNode->NodeGuid.ToString());
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
	}

	// Get position parameters (optional)
	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Get the event graph
	UEdGraph* EventGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
	}

	// Check if event already exists
	for (UEdGraphNode* Node : EventGraph->Nodes)
	{
		UK2Node_Event* ExistingEvent = Cast<UK2Node_Event>(Node);
		if (ExistingEvent && ExistingEvent->EventReference.GetMemberName() == FName(*EventName))
		{
			TSharedPtr<FJsonObject> ResultObj = FCommonUtils::CreateNodeResponse(ExistingEvent);
			ResultObj->SetBoolField(TEXT("already_exists"), true);
			return ResultObj;
		}
	}

	// Create the event node
	UK2Node_Event* EventNode = FCommonUtils::CreateEventNode(EventGraph, EventName, NodePosition);
	if (!EventNode)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create event node"));
	}

	// Mark the blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	return FCommonUtils::CreateNodeResponse(EventNode);
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddCustomEventNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString EventName;
	if (!Params->TryGetStringField(TEXT("event_name"), EventName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
	}

	FString Action;
	if (!Params->TryGetStringField(TEXT("action"), Action))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'action' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* TargetGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		TargetGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	}
	if (!TargetGraph)
	{
		TargetGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	}
	if (!TargetGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get target graph"));
	}

	if (Action == TEXT("define"))
	{
		// Check if custom event already exists
		for (UEdGraphNode* Node : TargetGraph->Nodes)
		{
			UK2Node_CustomEvent* ExistingEvent = Cast<UK2Node_CustomEvent>(Node);
			if (ExistingEvent && ExistingEvent->CustomFunctionName == FName(*EventName))
			{
				TSharedPtr<FJsonObject> ResultObj = FCommonUtils::CreateNodeResponse(ExistingEvent);
				ResultObj->SetBoolField(TEXT("already_exists"), true);
				return ResultObj;
			}
		}

		// Create new custom event
		UK2Node_CustomEvent* CustomEventNode = NewObject<UK2Node_CustomEvent>(TargetGraph);
		CustomEventNode->CreateNewGuid();
		CustomEventNode->CustomFunctionName = FName(*EventName);
		CustomEventNode->NodePosX = NodePosition.X;
		CustomEventNode->NodePosY = NodePosition.Y;
		TargetGraph->AddNode(CustomEventNode, true);
		CustomEventNode->PostPlacedNewNode();
		CustomEventNode->AllocateDefaultPins();

		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		return FCommonUtils::CreateNodeResponse(CustomEventNode);
	}
	else if (Action == TEXT("call"))
	{
		// Find existing custom event to call
		UK2Node_CustomEvent* FoundEvent = nullptr;
		for (UEdGraphNode* Node : TargetGraph->Nodes)
		{
			UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node);
			if (CustomEvent && CustomEvent->CustomFunctionName == FName(*EventName))
			{
				FoundEvent = CustomEvent;
				break;
			}
		}

		if (!FoundEvent)
		{
			return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Custom event '%s' not found"), *EventName));
		}

		// Create call function node with self member reference
		UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(TargetGraph);
		CallNode->CreateNewGuid();
		CallNode->FunctionReference.SetSelfMember(FName(*EventName));
		CallNode->NodePosX = NodePosition.X;
		CallNode->NodePosY = NodePosition.Y;
		TargetGraph->AddNode(CallNode, true);
		CallNode->PostPlacedNewNode();
		CallNode->AllocateDefaultPins();

		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		return FCommonUtils::CreateNodeResponse(CallNode);
	}
	else
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid action: %s (use 'define' or 'call')"), *Action));
	}
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	// Detect common mistakes: these are node classes, not functions
	if (FunctionName.Contains(TEXT("SpawnActor")) ||
		FunctionName == TEXT("Branch") ||
		FunctionName == TEXT("Sequence") ||
		FunctionName.Contains(TEXT("ForEach")) ||
		FunctionName.Contains(TEXT("MakeStruct")) ||
		FunctionName.Contains(TEXT("BreakStruct")) ||
		FunctionName == TEXT("Cast"))
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("'%s' is not a function - use add_blueprint_generic_node or add_blueprint_flow_control_node instead"),
			*FunctionName));
	}

	// Get position parameters (optional)
	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	// Get target class name for function lookup
	FString TargetClassName;
	Params->TryGetStringField(TEXT("target_class"), TargetClassName);

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Get auto_connect_self parameter
	bool bAutoConnectSelf = false;
	Params->TryGetBoolField(TEXT("auto_connect_self"), bAutoConnectSelf);

	// Get graph_name parameter
	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Get target graph
	UEdGraph* EventGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		EventGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	}
	if (!EventGraph)
	{
		EventGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	}
	if (!EventGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get target graph"));
	}

	// Find the function
	UFunction* Function = nullptr;
	UK2Node_CallFunction* FunctionNode = nullptr;
	FString ActualFunctionName = FunctionName;

	// Find function in specified target class
	if (!TargetClassName.IsEmpty())
	{
		UClass* TargetClass = FCommonUtils::FindClassByName(TargetClassName);
		if (TargetClass)
		{
			Function = TargetClass->FindFunctionByName(*FunctionName);
			// Retry with K2_ prefix
			if (!Function && !FunctionName.StartsWith(TEXT("K2_")))
			{
				FString K2Name = TEXT("K2_") + FunctionName;
				Function = TargetClass->FindFunctionByName(*K2Name);
				if (Function)
				{
					ActualFunctionName = K2Name;
					UE_LOG(LogTemp, Display, TEXT("AddBlueprintFunctionNode: Using K2_ variant '%s' instead of '%s'"), *K2Name, *FunctionName);
				}
			}
		}
	}

	// Fallback: search in Blueprint's generated class
	if (!Function)
	{
		Function = Blueprint->GeneratedClass->FindFunctionByName(*FunctionName);
		// Retry with K2_ prefix
		if (!Function && !FunctionName.StartsWith(TEXT("K2_")))
		{
			FString K2Name = TEXT("K2_") + FunctionName;
			Function = Blueprint->GeneratedClass->FindFunctionByName(*K2Name);
			if (Function)
			{
				ActualFunctionName = K2Name;
				UE_LOG(LogTemp, Display, TEXT("AddBlueprintFunctionNode: Using K2_ variant '%s' instead of '%s'"), *K2Name, *FunctionName);
			}
		}
	}

	// Create the function call node if we found the function
	if (Function)
	{
		FunctionNode = FCommonUtils::CreateFunctionCallNode(EventGraph, Function, NodePosition);
	}

	if (!FunctionNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s (also tried K2_%s)"), *FunctionName, *FunctionName));
	}

	// Auto-connect Self node to Target pin if requested
	bool bSelfConnected = false;
	if (bAutoConnectSelf)
	{
		// Find Target/self pin on function node
		UEdGraphPin* TargetPin = FCommonUtils::FindPin(FunctionNode, TEXT("self"), EGPD_Input);
		if (!TargetPin)
		{
			TargetPin = FCommonUtils::FindPin(FunctionNode, TEXT("Target"), EGPD_Input);
		}

		if (TargetPin)
		{
			// Create Self reference node
			FVector2D SelfPosition(NodePosition.X - 150, NodePosition.Y + 50);
			UK2Node_Self* SelfNode = FCommonUtils::CreateSelfReferenceNode(EventGraph, SelfPosition);
			if (SelfNode)
			{
				// Find Self output pin
				UEdGraphPin* SelfOutputPin = nullptr;
				for (UEdGraphPin* Pin : SelfNode->Pins)
				{
					if (Pin->Direction == EGPD_Output)
					{
						SelfOutputPin = Pin;
						break;
					}
				}

				if (SelfOutputPin)
				{
					SelfOutputPin->MakeLinkTo(TargetPin);
					bSelfConnected = true;
				}
			}
		}
	}

	// Set parameters if provided
	if (Params->HasField(TEXT("params")))
	{
		const TSharedPtr<FJsonObject>* ParamsObj;
		if (Params->TryGetObjectField(TEXT("params"), ParamsObj))
		{
			// Process parameters
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Param : (*ParamsObj)->Values)
			{
				const FString& ParamName = Param.Key;
				const TSharedPtr<FJsonValue>& ParamValue = Param.Value;

				// Find the parameter pin
				UEdGraphPin* ParamPin = FCommonUtils::FindPin(FunctionNode, ParamName, EGPD_Input);
				if (ParamPin)
				{
					const FName& PinCategory = ParamPin->PinType.PinCategory;

					// Handle Class/Object types
					if (ParamValue->Type == EJson::String)
					{
						FString ValueStr = ParamValue->AsString();

						if (PinCategory == UEdGraphSchema_K2::PC_Class || PinCategory == UEdGraphSchema_K2::PC_SoftClass)
						{
							UClass* FoundClass = nullptr;
							if (ValueStr.Contains(TEXT("/")))
							{
								UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *ValueStr);
								if (BP && BP->GeneratedClass)
								{
									FoundClass = BP->GeneratedClass;
								}
								else
								{
									FoundClass = LoadClass<UObject>(nullptr, *ValueStr);
								}
							}
							else
							{
								FoundClass = FCommonUtils::FindClassByName(ValueStr);
							}
							if (FoundClass)
							{
								ParamPin->DefaultObject = FoundClass;
								ParamPin->DefaultValue = FoundClass->GetPathName();
							}
						}
						else if (PinCategory == UEdGraphSchema_K2::PC_Object || PinCategory == UEdGraphSchema_K2::PC_SoftObject)
						{
							UObject* FoundObject = LoadObject<UObject>(nullptr, *ValueStr);
							if (FoundObject)
							{
								ParamPin->DefaultObject = FoundObject;
								ParamPin->DefaultValue = FoundObject->GetPathName();
							}
						}
						else
						{
							ParamPin->DefaultValue = ValueStr;
						}
					}
					else if (ParamValue->Type == EJson::Number)
					{
						if (PinCategory == UEdGraphSchema_K2::PC_Int)
						{
							int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
							ParamPin->DefaultValue = FString::FromInt(IntValue);
						}
						else
						{
							float FloatValue = ParamValue->AsNumber();
							ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
						}
					}
					else if (ParamValue->Type == EJson::Boolean)
					{
						bool BoolValue = ParamValue->AsBool();
						ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
					}
					else if (ParamValue->Type == EJson::Array)
					{
						const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
						if (ParamValue->TryGetArray(ArrayValue) && ArrayValue->Num() == 3)
						{
							if (PinCategory == UEdGraphSchema_K2::PC_Struct &&
								ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
							{
								float X = (*ArrayValue)[0]->AsNumber();
								float Y = (*ArrayValue)[1]->AsNumber();
								float Z = (*ArrayValue)[2]->AsNumber();
								FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
								ParamPin->DefaultValue = VectorString;
							}
						}
					}
				}
			}
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	// Build context with self_connected info if applicable
	FInstructionContext Ctx;
	if (bAutoConnectSelf && bSelfConnected)
	{
		Ctx.Type = TEXT("self_auto_connected");
		Ctx.Message = TEXT("Self reference automatically connected to Target pin.");
	}

	TSharedPtr<FJsonObject> ResultObj = FCommonUtils::CreateNodeResponseWithContext(FunctionNode, Ctx);
	if (bAutoConnectSelf)
	{
		ResultObj->SetBoolField(TEXT("self_connected"), bSelfConnected);
	}
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	FString VariableType;
	if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
	}

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	FString SubType;
	Params->TryGetStringField(TEXT("sub_type"), SubType);

	// Auto-convert common class types to Object + sub_type
	// Actor, Pawn, Character, etc. are shortcuts for Object references
	if (VariableType == TEXT("Actor") ||
		VariableType == TEXT("Pawn") ||
		VariableType == TEXT("Character") ||
		VariableType == TEXT("Controller") ||
		VariableType == TEXT("PlayerController") ||
		VariableType == TEXT("ActorComponent"))
	{
		SubType = VariableType;
		VariableType = TEXT("Object");
	}

	FEdGraphPinType PinType;

	if (VariableType == TEXT("Boolean"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (VariableType == TEXT("Integer") || VariableType == TEXT("Int"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (VariableType == TEXT("Float") || VariableType == TEXT("Double"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		PinType.PinSubCategory = VariableType == TEXT("Double") ? TEXT("double") : TEXT("float");
	}
	else if (VariableType == TEXT("String"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (VariableType == TEXT("Name"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
	}
	else if (VariableType == TEXT("Text"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
	}
	else if (VariableType == TEXT("Vector"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
	}
	else if (VariableType == TEXT("Rotator"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
	}
	else if (VariableType == TEXT("Transform"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
	}
	else if (VariableType == TEXT("GameplayTag"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = FGameplayTag::StaticStruct();
	}
	else if (VariableType == TEXT("GameplayTagContainer"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = FGameplayTagContainer::StaticStruct();
	}
	else if (VariableType == TEXT("Object"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
		if (!SubType.IsEmpty())
		{
			UClass* ObjectClass = FCommonUtils::FindClassByName(SubType);
			if (ObjectClass)
			{
				PinType.PinSubCategoryObject = ObjectClass;
			}
			else
			{
				return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Object sub_type class not found: %s"), *SubType));
			}
		}
		else
		{
			PinType.PinSubCategoryObject = UObject::StaticClass();
		}
	}
	else if (VariableType == TEXT("Class"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Class;
		if (!SubType.IsEmpty())
		{
			UClass* MetaClass = FCommonUtils::FindClassByName(SubType);
			if (MetaClass)
			{
				PinType.PinSubCategoryObject = MetaClass;
			}
			else
			{
				return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Class sub_type not found: %s"), *SubType));
			}
		}
		else
		{
			PinType.PinSubCategoryObject = UObject::StaticClass();
		}
	}
	else if (VariableType == TEXT("SoftObject"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_SoftObject;
		if (!SubType.IsEmpty())
		{
			UClass* ObjectClass = FCommonUtils::FindClassByName(SubType);
			if (ObjectClass)
			{
				PinType.PinSubCategoryObject = ObjectClass;
			}
		}
	}
	else if (VariableType == TEXT("SoftClass"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_SoftClass;
		if (!SubType.IsEmpty())
		{
			UClass* MetaClass = FCommonUtils::FindClassByName(SubType);
			if (MetaClass)
			{
				PinType.PinSubCategoryObject = MetaClass;
			}
		}
	}
	else if (VariableType == TEXT("Struct"))
	{
		if (SubType.IsEmpty())
		{
			return FCommonUtils::CreateErrorResponse(TEXT("Struct type requires 'sub_type' parameter"));
		}
		UScriptStruct* FoundStruct = FindFirstObject<UScriptStruct>(*SubType, EFindFirstObjectOptions::None);
		if (FoundStruct)
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			PinType.PinSubCategoryObject = FoundStruct;
		}
		else
		{
			return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Struct not found: %s"), *SubType));
		}
	}
	else
	{
		// Try to find as struct type
		UScriptStruct* FoundStruct = FindFirstObject<UScriptStruct>(*VariableType, EFindFirstObjectOptions::None);
		if (FoundStruct)
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			PinType.PinSubCategoryObject = FoundStruct;
		}
		else
		{
			return FCommonUtils::CreateErrorResponse(FString::Printf(
				TEXT("Unsupported variable type: %s. Supported: Boolean, Integer, Float, Double, String, Name, Text, Vector, Rotator, Transform, GameplayTag, GameplayTagContainer, Object, Class, SoftObject, SoftClass, Struct (with sub_type)"), *VariableType));
		}
	}

	// Create the variable
	FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

	// Set variable properties
	FBPVariableDescription* NewVar = nullptr;
	for (FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		if (Variable.VarName == FName(*VariableName))
		{
			NewVar = &Variable;
			break;
		}
	}

	if (NewVar)
	{
		// Process metadata object for UPROPERTY specifiers
		if (Params->HasField(TEXT("metadata")))
		{
			const TSharedPtr<FJsonObject>* MetaObjPtr;
			if (Params->TryGetObjectField(TEXT("metadata"), MetaObjPtr) && MetaObjPtr)
			{
				TSharedPtr<FJsonObject> MetaObj = *MetaObjPtr;
				for (const auto& Pair : MetaObj->Values)
				{
					FString Key = Pair.Key;
					FString Value;
					Pair.Value->TryGetString(Value);

					// Helper lambda for boolean check
					auto IsTrueValue = [&Value]() {
						return Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value == TEXT("1");
					};

					// =====================================================================
					// PropertyFlags - Visibility & Access
					// =====================================================================
					if (Key.Equals(TEXT("BlueprintReadOnly"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_BlueprintVisible | CPF_BlueprintReadOnly;
						}
					}
					else if (Key.Equals(TEXT("BlueprintReadWrite"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_BlueprintVisible;
							NewVar->PropertyFlags &= ~CPF_BlueprintReadOnly;
						}
					}
					else if (Key.Equals(TEXT("VisibleAnywhere"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_Edit | CPF_EditConst;
						}
					}
					else if (Key.Equals(TEXT("VisibleDefaultsOnly"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_Edit | CPF_EditConst | CPF_DisableEditOnInstance;
						}
					}
					else if (Key.Equals(TEXT("VisibleInstanceOnly"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_Edit | CPF_EditConst | CPF_DisableEditOnTemplate;
						}
					}
					else if (Key.Equals(TEXT("EditAnywhere"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_Edit;
						}
					}
					else if (Key.Equals(TEXT("EditDefaultsOnly"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_Edit | CPF_DisableEditOnInstance;
						}
					}
					else if (Key.Equals(TEXT("EditInstanceOnly"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_Edit | CPF_DisableEditOnTemplate;
						}
					}
					// =====================================================================
					// PropertyFlags - Spawning & Instantiation
					// =====================================================================
					else if (Key.Equals(TEXT("ExposeOnSpawn"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_ExposeOnSpawn | CPF_BlueprintVisible;
						}
					}
					else if (Key.Equals(TEXT("Instanced"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_PersistentInstance | CPF_ExportObject | CPF_InstancedReference;
						}
					}
					// =====================================================================
					// PropertyFlags - Replication
					// =====================================================================
					else if (Key.Equals(TEXT("Replicated"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_Net;
							NewVar->ReplicationCondition = COND_None;
						}
					}
					else if (Key.Equals(TEXT("ReplicatedUsing"), ESearchCase::IgnoreCase))
					{
						NewVar->PropertyFlags |= CPF_Net | CPF_RepNotify;
						NewVar->RepNotifyFunc = FName(*Value);
						NewVar->ReplicationCondition = COND_None;
					}
					else if (Key.Equals(TEXT("NotReplicated"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_RepSkip;
						}
					}
					else if (Key.Equals(TEXT("ReplicationCondition"), ESearchCase::IgnoreCase))
					{
						if (Value.Equals(TEXT("InitialOnly"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_InitialOnly;
						}
						else if (Value.Equals(TEXT("OwnerOnly"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_OwnerOnly;
						}
						else if (Value.Equals(TEXT("SkipOwner"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_SkipOwner;
						}
						else if (Value.Equals(TEXT("SimulatedOnly"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_SimulatedOnly;
						}
						else if (Value.Equals(TEXT("AutonomousOnly"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_AutonomousOnly;
						}
						else if (Value.Equals(TEXT("SimulatedOrPhysics"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_SimulatedOrPhysics;
						}
						else if (Value.Equals(TEXT("InitialOrOwner"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_InitialOrOwner;
						}
						else if (Value.Equals(TEXT("Custom"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_Custom;
						}
						else if (Value.Equals(TEXT("ReplayOrOwner"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_ReplayOrOwner;
						}
						else if (Value.Equals(TEXT("ReplayOnly"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_ReplayOnly;
						}
						else if (Value.Equals(TEXT("SkipReplay"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_SkipReplay;
						}
						else if (Value.Equals(TEXT("Never"), ESearchCase::IgnoreCase))
						{
							NewVar->ReplicationCondition = COND_Never;
						}
						else
						{
							NewVar->ReplicationCondition = COND_None;
						}
					}
					// =====================================================================
					// PropertyFlags - Serialization & Persistence
					// =====================================================================
					else if (Key.Equals(TEXT("SaveGame"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_SaveGame;
						}
					}
					else if (Key.Equals(TEXT("Transient"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_Transient;
						}
					}
					else if (Key.Equals(TEXT("DuplicateTransient"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_DuplicateTransient;
						}
					}
					else if (Key.Equals(TEXT("NonPIEDuplicateTransient"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_NonPIEDuplicateTransient;
						}
					}
					else if (Key.Equals(TEXT("SkipSerialization"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_SkipSerialization;
						}
					}
					// =====================================================================
					// PropertyFlags - Advanced / Editor
					// =====================================================================
					else if (Key.Equals(TEXT("AdvancedDisplay"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_AdvancedDisplay;
						}
					}
					else if (Key.Equals(TEXT("AssetRegistrySearchable"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_AssetRegistrySearchable;
						}
					}
					else if (Key.Equals(TEXT("SimpleDisplay"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_SimpleDisplay;
						}
					}
					else if (Key.Equals(TEXT("Interp"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_Edit | CPF_BlueprintVisible | CPF_Interp;
						}
					}
					else if (Key.Equals(TEXT("NonTransactional"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_NonTransactional;
						}
					}
					else if (Key.Equals(TEXT("NoClear"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_NoClear;
						}
					}
					else if (Key.Equals(TEXT("TextExportTransient"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_TextExportTransient;
						}
					}
					// =====================================================================
					// PropertyFlags - Delegates (GAS multicast delegates)
					// =====================================================================
					else if (Key.Equals(TEXT("BlueprintAssignable"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_BlueprintAssignable;
						}
					}
					else if (Key.Equals(TEXT("BlueprintCallable"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_BlueprintCallable;
						}
					}
					else if (Key.Equals(TEXT("BlueprintAuthorityOnly"), ESearchCase::IgnoreCase))
					{
						if (IsTrueValue())
						{
							NewVar->PropertyFlags |= CPF_BlueprintAuthorityOnly;
						}
					}
					// =====================================================================
					// Direct Category Assignment
					// =====================================================================
					else if (Key.Equals(TEXT("Category"), ESearchCase::IgnoreCase))
					{
						NewVar->Category = FText::FromString(Value);
					}
					// =====================================================================
					// Known MetaData keys (string values set via SetBlueprintVariableMetaData)
					// =====================================================================
					else if (Key.Equals(TEXT("Tooltip"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("DisplayName"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("ClampMin"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("ClampMax"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("UIMin"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("UIMax"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("MakeEditWidget"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("AllowPrivateAccess"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("GetOptions"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("EditCondition"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("EditConditionHides"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("InlineEditConditionToggle"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("Units"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("ForceUnits"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("Delta"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("LinearDeltaSensitivity"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("ArrayClamp"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("TitleProperty"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("NoResetToDefault"), ESearchCase::IgnoreCase) ||
							 Key.Equals(TEXT("HideAlphaChannel"), ESearchCase::IgnoreCase))
					{
						FBlueprintEditorUtils::SetBlueprintVariableMetaData(
							Blueprint, FName(*VariableName), nullptr, FName(*Key), Value);
					}
					// =====================================================================
					// Unknown key - reject with error
					// =====================================================================
					else
					{
						return FCommonUtils::CreateErrorResponse(FString::Printf(
							TEXT("Unknown metadata key: '%s'. Valid keys: BlueprintReadOnly, BlueprintReadWrite, EditAnywhere, EditDefaultsOnly, EditInstanceOnly, VisibleAnywhere, VisibleDefaultsOnly, VisibleInstanceOnly, ExposeOnSpawn, Instanced, Replicated, ReplicatedUsing, NotReplicated, ReplicationCondition, SaveGame, Transient, DuplicateTransient, SkipSerialization, AdvancedDisplay, Interp, SimpleDisplay, NoClear, Category, Tooltip, DisplayName, ClampMin, ClampMax, UIMin, UIMax, MakeEditWidget, AllowPrivateAccess, GetOptions, EditCondition, Units"),
							*Key));
					}
				}
			}
		}
	}

	// Mark the blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("variable_name"), VariableName);
	ResultObj->SetStringField(TEXT("variable_type"), VariableType);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString ActionName;
	if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
	}

	// Get position parameters (optional)
	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Get the event graph
	UEdGraph* EventGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
	}

	// Create the input action node
	UK2Node_InputAction* InputActionNode = FCommonUtils::CreateInputActionNode(EventGraph, ActionName, NodePosition);
	if (!InputActionNode)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create input action node"));
	}

	// Mark the blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), InputActionNode->NodeGuid.ToString());
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	// Get position parameters (optional)
	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Get target graph
	UEdGraph* EventGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		EventGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	}
	if (!EventGraph)
	{
		EventGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	}
	if (!EventGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get target graph"));
	}

	// Create the self node
	UK2Node_Self* SelfNode = FCommonUtils::CreateSelfReferenceNode(EventGraph, NodePosition);
	if (!SelfNode)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create self node"));
	}

	// Mark the blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), SelfNode->NodeGuid.ToString());
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleListBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Get target graph (default: EventGraph)
	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	UEdGraph* TargetGraph = nullptr;
	if (GraphName.IsEmpty())
	{
		TargetGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	}
	else
	{
		TargetGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	}

	if (!TargetGraph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	// Filter parameters (all optional)
	FString NodeType;
	Params->TryGetStringField(TEXT("node_type"), NodeType);

	FString EventName;
	Params->TryGetStringField(TEXT("event_name"), EventName);

	FString NodeTitle;
	Params->TryGetStringField(TEXT("node_title"), NodeTitle);

	FString NodeClass;
	Params->TryGetStringField(TEXT("node_class"), NodeClass);

	bool bHasUnconnectedPins = false;
	Params->TryGetBoolField(TEXT("has_unconnected_pins"), bHasUnconnectedPins);

	bool bHasUnconnectedExecPins = false;
	Params->TryGetBoolField(TEXT("has_unconnected_exec_pins"), bHasUnconnectedExecPins);

	bool bHasUnconnectedDataPins = false;
	Params->TryGetBoolField(TEXT("has_unconnected_data_pins"), bHasUnconnectedDataPins);

	int32 Limit = 50;
	Params->TryGetNumberField(TEXT("limit"), Limit);

	FString SortBy;
	Params->TryGetStringField(TEXT("sort_by"), SortBy);

	TArray<UEdGraphNode*> MatchingNodes;

	for (UEdGraphNode* Node : TargetGraph->Nodes)
	{
		if (!Node) continue;

		bool bMatches = true;

		if (!NodeType.IsEmpty())
		{
			if (NodeType == TEXT("Event"))
			{
				if (!Cast<UK2Node_Event>(Node)) bMatches = false;
			}
			else if (NodeType == TEXT("Function"))
			{
				if (!Cast<UK2Node_CallFunction>(Node)) bMatches = false;
			}
			else if (NodeType == TEXT("Variable"))
			{
				if (!Cast<UK2Node_VariableGet>(Node) && !Node->GetClass()->GetName().Contains(TEXT("VariableSet")))
					bMatches = false;
			}
			else if (NodeType == TEXT("FlowControl"))
			{
				if (!Cast<UK2Node_IfThenElse>(Node) && !Cast<UK2Node_ExecutionSequence>(Node))
					bMatches = false;
			}
		}

		if (bMatches && !EventName.IsEmpty())
		{
			UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
			if (!EventNode || EventNode->EventReference.GetMemberName() != FName(*EventName))
			{
				bMatches = false;
			}
		}

		if (bMatches && !NodeTitle.IsEmpty())
		{
			FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
			if (!Title.Contains(NodeTitle, ESearchCase::IgnoreCase))
			{
				bMatches = false;
			}
		}

		if (bMatches && !NodeClass.IsEmpty())
		{
			if (Node->GetClass()->GetName() != NodeClass)
			{
				bMatches = false;
			}
		}

		if (bMatches && bHasUnconnectedPins)
		{
			bool bFoundUnconnected = false;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && !Pin->bHidden && Pin->LinkedTo.Num() == 0)
				{
					bFoundUnconnected = true;
					break;
				}
			}
			if (!bFoundUnconnected) bMatches = false;
		}

		if (bMatches && bHasUnconnectedExecPins)
		{
			bool bFoundUnconnected = false;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && !Pin->bHidden && Pin->LinkedTo.Num() == 0 &&
					Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				{
					bFoundUnconnected = true;
					break;
				}
			}
			if (!bFoundUnconnected) bMatches = false;
		}

		if (bMatches && bHasUnconnectedDataPins)
		{
			bool bFoundUnconnected = false;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && !Pin->bHidden && Pin->LinkedTo.Num() == 0 &&
					Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
				{
					bFoundUnconnected = true;
					break;
				}
			}
			if (!bFoundUnconnected) bMatches = false;
		}

		if (bMatches)
		{
			MatchingNodes.Add(Node);
		}
	}

	if (SortBy == TEXT("position_x"))
	{
		MatchingNodes.Sort([](const UEdGraphNode& A, const UEdGraphNode& B) { return A.NodePosX < B.NodePosX; });
	}
	else if (SortBy == TEXT("position_y"))
	{
		MatchingNodes.Sort([](const UEdGraphNode& A, const UEdGraphNode& B) { return A.NodePosY < B.NodePosY; });
	}
	else if (SortBy == TEXT("newest") || SortBy == TEXT("rightmost"))
	{
		MatchingNodes.Sort([](const UEdGraphNode& A, const UEdGraphNode& B) { return A.NodePosX > B.NodePosX; });
	}

	if (MatchingNodes.Num() > Limit)
	{
		MatchingNodes.SetNum(Limit);
	}

	TArray<TSharedPtr<FJsonValue>> NodesArray;
	for (UEdGraphNode* Node : MatchingNodes)
	{
		TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
		NodeObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
		NodeObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
		NodeObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
		NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
		NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);

		TArray<TSharedPtr<FJsonValue>> PinsArray;
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

			if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
			{
				UEdGraphNode* ConnectedNode = Pin->LinkedTo[0]->GetOwningNode();
				if (ConnectedNode)
				{
					PinObj->SetStringField(TEXT("connected_to_node"), ConnectedNode->NodeGuid.ToString());
					PinObj->SetStringField(TEXT("connected_to_pin"), Pin->LinkedTo[0]->PinName.ToString());
				}
			}

			PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
		}
		NodeObj->SetArrayField(TEXT("pins"), PinsArray);

		NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetArrayField(TEXT("nodes"), NodesArray);
	ResultObj->SetNumberField(TEXT("total_found"), MatchingNodes.Num());
	ResultObj->SetBoolField(TEXT("success"), true);

	return ResultObj;
}

// ============================================================================
// Material Commands
// ============================================================================

TSharedPtr<FJsonObject> FBlueprintCommands::HandleApplyMaterialToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
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

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Find the component
	USCS_Node* ComponentNode = nullptr;
	if (!Blueprint->SimpleConstructionScript)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Invalid blueprint construction script"));
	}

	for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
	{
		if (Node && Node->GetVariableName().ToString() == ComponentName)
		{
			ComponentNode = Node;
			break;
		}
	}

	if (!ComponentNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	// Cast to PrimitiveComponent
	UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
	if (!PrimComponent)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
	}

	// Load the material asset from Content Browser
	UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
	if (!Material)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
	}

	// Apply the material directly (no dynamic material instance - this is the key fix!)
	PrimComponent->SetMaterial(MaterialSlot, Material);

	// Mark the blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	UE_LOG(LogTemp, Display, TEXT("FBlueprintCommands::HandleApplyMaterialToBlueprint: Applied material '%s' to component '%s' slot %d in blueprint '%s'"),
		*MaterialPath, *ComponentName, MaterialSlot, *BlueprintName);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
	ResultObj->SetStringField(TEXT("component"), ComponentName);
	ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
	ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleGetBlueprintMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
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

	// Get optional path parameter (default: /Game/Blueprints/)
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Find the component
	USCS_Node* ComponentNode = nullptr;
	if (!Blueprint->SimpleConstructionScript)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Invalid blueprint construction script"));
	}

	for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
	{
		if (Node && Node->GetVariableName().ToString() == ComponentName)
		{
			ComponentNode = Node;
			break;
		}
	}

	if (!ComponentNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	// Cast to PrimitiveComponent
	UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
	if (!PrimComponent)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
	}

	// Get material information
	TArray<TSharedPtr<FJsonValue>> MaterialsArray;
	int32 NumMaterials = PrimComponent->GetNumMaterials();

	for (int32 i = 0; i < NumMaterials; i++)
	{
		TSharedPtr<FJsonObject> MaterialInfo = MakeShared<FJsonObject>();
		MaterialInfo->SetNumberField(TEXT("slot"), i);

		UMaterialInterface* Material = PrimComponent->GetMaterial(i);
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

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
	ResultObj->SetStringField(TEXT("component"), ComponentName);
	ResultObj->SetNumberField(TEXT("material_count"), NumMaterials);
	ResultObj->SetArrayField(TEXT("materials"), MaterialsArray);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

// ============================================================================
// Organization Commands
// ============================================================================

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddCommentBox(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString CommentText;
	if (!Params->TryGetStringField(TEXT("comment_text"), CommentText))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'comment_text' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* EventGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	if (!EventGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
	}

	// Get position and size
	FVector2D Position(0.0f, 0.0f);
	if (Params->HasField(TEXT("position")))
	{
		Position = FCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
	}

	FVector2D Size(400.0f, 200.0f);
	if (Params->HasField(TEXT("size")))
	{
		Size = FCommonUtils::GetVector2DFromJson(Params, TEXT("size"));
	}

	// Create comment node
	UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(EventGraph);
	CommentNode->NodeComment = CommentText;
	CommentNode->NodePosX = Position.X;
	CommentNode->NodePosY = Position.Y;
	CommentNode->NodeWidth = Size.X;
	CommentNode->NodeHeight = Size.Y;

	EventGraph->AddNode(CommentNode);
	CommentNode->CreateNewGuid();
	CommentNode->PostPlacedNewNode();

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), CommentNode->NodeGuid.ToString());
	ResultObj->SetStringField(TEXT("comment_text"), CommentText);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAnalyzeBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	bool bIncludeAllGraphs = true;
	Params->TryGetBoolField(TEXT("include_all_graphs"), bIncludeAllGraphs);

	bool bDetailedPins = true;
	Params->TryGetBoolField(TEXT("detailed_pins"), bDetailedPins);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Get graphs to analyze
	TArray<UEdGraph*> GraphsToAnalyze;
	if (bIncludeAllGraphs)
	{
		GraphsToAnalyze = FCommonUtils::GetAllGraphs(Blueprint);
	}
	else
	{
		UEdGraph* EventGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
		if (EventGraph) GraphsToAnalyze.Add(EventGraph);
	}

	// Collect graph information
	TArray<TSharedPtr<FJsonValue>> GraphsArray;
	TMap<FString, int32> NodeTypeCounts;
	int32 TotalNodeCount = 0;

	for (UEdGraph* Graph : GraphsToAnalyze)
	{
		if (!Graph) continue;

		TSharedPtr<FJsonObject> GraphInfo = FCommonUtils::GraphToJson(Graph);

		// Collect nodes for this graph
		TArray<TSharedPtr<FJsonValue>> NodesArray;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;

			TSharedPtr<FJsonObject> NodeInfo = MakeShared<FJsonObject>();
			NodeInfo->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
			NodeInfo->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());
			NodeInfo->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			NodeInfo->SetNumberField(TEXT("pos_x"), Node->NodePosX);
			NodeInfo->SetNumberField(TEXT("pos_y"), Node->NodePosY);

			if (bDetailedPins)
			{
				NodeInfo->SetArrayField(TEXT("pins"), FCommonUtils::NodePinsToJson(Node));
			}

			NodesArray.Add(MakeShared<FJsonValueObject>(NodeInfo));

			FString NodeClass = Node->GetClass()->GetName();
			NodeTypeCounts.FindOrAdd(NodeClass)++;
			TotalNodeCount++;
		}

		GraphInfo->SetArrayField(TEXT("nodes"), NodesArray);
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphInfo));
	}

	// Build type summary
	TArray<TSharedPtr<FJsonValue>> TypeSummaryArray;
	for (const TPair<FString, int32>& Pair : NodeTypeCounts)
	{
		TSharedPtr<FJsonObject> TypeInfo = MakeShared<FJsonObject>();
		TypeInfo->SetStringField(TEXT("type"), Pair.Key);
		TypeInfo->SetNumberField(TEXT("count"), Pair.Value);
		TypeSummaryArray.Add(MakeShared<FJsonValueObject>(TypeInfo));
	}

	// Collect components
	TArray<TSharedPtr<FJsonValue>> ComponentsArray;
	if (Blueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (!Node) continue;

			TSharedPtr<FJsonObject> CompInfo = MakeShared<FJsonObject>();
			CompInfo->SetStringField(TEXT("name"), Node->GetVariableName().ToString());
			CompInfo->SetStringField(TEXT("class"), Node->ComponentClass ? Node->ComponentClass->GetName() : TEXT("Unknown"));
			ComponentsArray.Add(MakeShared<FJsonValueObject>(CompInfo));
		}
	}

	// Collect variables
	TArray<TSharedPtr<FJsonValue>> VariablesArray;
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		TSharedPtr<FJsonObject> VarInfo = MakeShared<FJsonObject>();
		VarInfo->SetStringField(TEXT("name"), Var.VarName.ToString());
		VarInfo->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());
		if (Var.VarType.PinSubCategoryObject.IsValid())
		{
			VarInfo->SetStringField(TEXT("sub_type"), Var.VarType.PinSubCategoryObject->GetName());
		}
		VariablesArray.Add(MakeShared<FJsonValueObject>(VarInfo));
	}

	// Collect overridable functions from parent class
	TArray<TSharedPtr<FJsonValue>> OverridablesArray;
	if (Blueprint->ParentClass)
	{
		for (TFieldIterator<UFunction> It(Blueprint->ParentClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
		{
			UFunction* Func = *It;
			if (Func && Func->HasAnyFunctionFlags(FUNC_BlueprintEvent))
			{
				OverridablesArray.Add(MakeShared<FJsonValueString>(Func->GetName()));
			}
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
	ResultObj->SetStringField(TEXT("parent_class"), Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None"));
	ResultObj->SetNumberField(TEXT("total_node_count"), TotalNodeCount);
	ResultObj->SetNumberField(TEXT("graph_count"), GraphsArray.Num());
	ResultObj->SetArrayField(TEXT("graphs"), GraphsArray);
	ResultObj->SetArrayField(TEXT("node_type_summary"), TypeSummaryArray);
	ResultObj->SetArrayField(TEXT("components"), ComponentsArray);
	ResultObj->SetArrayField(TEXT("variables"), VariablesArray);
	ResultObj->SetArrayField(TEXT("overridable_functions"), OverridablesArray);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

// ============================================================================
// GAS (Gameplay Ability System) Commands
// ============================================================================

TSharedPtr<FJsonObject> FBlueprintCommands::HandleCreateGameplayEffect(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString EffectName;
	if (!Params->TryGetStringField(TEXT("name"), EffectName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	// Get optional parameters
	FString AssetPath = TEXT("/Game/GAS/Effects/");
	Params->TryGetStringField(TEXT("asset_path"), AssetPath);

	FString ParentClass = TEXT("GameplayEffect");
	Params->TryGetStringField(TEXT("parent_class"), ParentClass);

	// Ensure path ends with /
	if (!AssetPath.EndsWith(TEXT("/")))
	{
		AssetPath += TEXT("/");
	}

	// Check if asset already exists - return existing if found
	FString FullAssetPath = AssetPath + EffectName;
	if (UEditorAssetLibrary::DoesAssetExist(FullAssetPath))
	{
		UBlueprint* ExistingEffect = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(FullAssetPath));
		if (ExistingEffect)
		{
			TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
			ResultObj->SetStringField(TEXT("name"), EffectName);
			ResultObj->SetStringField(TEXT("path"), FullAssetPath);
			ResultObj->SetBoolField(TEXT("success"), true);
			ResultObj->SetBoolField(TEXT("already_exists"), true);
			return ResultObj;
		}
	}

	// Load parent class
	UClass* ParentGEClass = nullptr;

	if (ParentClass == TEXT("GameplayEffect") || ParentClass == TEXT("UGameplayEffect"))
	{
		ParentGEClass = UGameplayEffect::StaticClass();
	}
	else
	{
		ParentGEClass = LoadClass<UGameplayEffect>(nullptr, *ParentClass);

		if (!ParentGEClass)
		{
			return FCommonUtils::CreateErrorResponse(FString::Printf(
				TEXT("Parent class not found: '%s'. Use search_assets to get the full class path."),
				*ParentClass));
		}
	}

	// Create the blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentGEClass;

	// Create the blueprint package
	UPackage* Package = CreatePackage(*(AssetPath + EffectName));
	UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(),
		Package,
		*EffectName,
		RF_Standalone | RF_Public,
		nullptr,
		GWarn
	));

	if (!NewBlueprint)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create GameplayEffect Blueprint"));
	}

	// Compile blueprint first to generate the class
	FKismetEditorUtilities::CompileBlueprint(NewBlueprint);

	// Get the CDO to set default properties
	UGameplayEffect* EffectCDO = NewBlueprint->GeneratedClass ?
		Cast<UGameplayEffect>(NewBlueprint->GeneratedClass->GetDefaultObject()) : nullptr;

	if (EffectCDO)
	{
		// Set DurationPolicy
		FString DurationPolicy;
		if (Params->TryGetStringField(TEXT("duration_policy"), DurationPolicy))
		{
			if (DurationPolicy == TEXT("Instant"))
			{
				EffectCDO->DurationPolicy = EGameplayEffectDurationType::Instant;
			}
			else if (DurationPolicy == TEXT("HasDuration"))
			{
				EffectCDO->DurationPolicy = EGameplayEffectDurationType::HasDuration;
			}
			else if (DurationPolicy == TEXT("Infinite"))
			{
				EffectCDO->DurationPolicy = EGameplayEffectDurationType::Infinite;
			}
		}

		// Set Duration value for HasDuration policy
		double Duration = 0.0;
		if (Params->TryGetNumberField(TEXT("duration"), Duration) && Duration > 0.0)
		{
			// Set duration using scalable float magnitude
			EffectCDO->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
		}

		// Set Period for DoT/HoT effects
		double Period = 0.0;
		if (Params->TryGetNumberField(TEXT("period"), Period) && Period > 0.0)
		{
			EffectCDO->Period = FScalableFloat(Period);
		}

		// Process Modifiers array
		const TArray<TSharedPtr<FJsonValue>>* ModifiersArray;
		if (Params->TryGetArrayField(TEXT("modifiers"), ModifiersArray))
		{
			for (const TSharedPtr<FJsonValue>& ModifierValue : *ModifiersArray)
			{
				const TSharedPtr<FJsonObject>* ModifierObj;
				if (ModifierValue->TryGetObject(ModifierObj))
				{
					FString AttributeName;
					FString OperationStr;
					double Value = 0.0;

					(*ModifierObj)->TryGetStringField(TEXT("attribute"), AttributeName);
					(*ModifierObj)->TryGetStringField(TEXT("operation"), OperationStr);
					(*ModifierObj)->TryGetNumberField(TEXT("value"), Value);

					if (!AttributeName.IsEmpty())
					{
						FGameplayModifierInfo ModifierInfo;

						// Find the attribute by name (searches all AttributeSets in the project)
						bool bFoundAttribute = false;
						for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
						{
							if (ClassIt->IsChildOf(UAttributeSet::StaticClass()) && !ClassIt->HasAnyClassFlags(CLASS_Abstract))
							{
								for (TFieldIterator<FProperty> PropIt(*ClassIt); PropIt; ++PropIt)
								{
									if (PropIt->GetName() == AttributeName)
									{
										FGameplayAttribute Attribute(*PropIt);
										ModifierInfo.Attribute = Attribute;
										bFoundAttribute = true;
										break;
									}
								}
								if (bFoundAttribute) break;
							}
						}

						if (bFoundAttribute)
						{
							// Set operation
							if (OperationStr == TEXT("Add"))
							{
								ModifierInfo.ModifierOp = EGameplayModOp::Additive;
							}
							else if (OperationStr == TEXT("Multiply"))
							{
								ModifierInfo.ModifierOp = EGameplayModOp::Multiplicitive;
							}
							else if (OperationStr == TEXT("Divide"))
							{
								ModifierInfo.ModifierOp = EGameplayModOp::Division;
							}
							else if (OperationStr == TEXT("Override"))
							{
								ModifierInfo.ModifierOp = EGameplayModOp::Override;
							}
							else
							{
								ModifierInfo.ModifierOp = EGameplayModOp::Additive; // Default
							}

							// Set magnitude
							ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Value));

							// Add to modifiers array
							EffectCDO->Modifiers.Add(ModifierInfo);
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("FBlueprintCommands::HandleCreateGameplayEffect: Attribute '%s' not found in any AttributeSet"), *AttributeName);
						}
					}
				}
			}
		}

		// Process GrantedTags
		const TArray<TSharedPtr<FJsonValue>>* GrantedTagsArray;
		if (Params->TryGetArrayField(TEXT("granted_tags"), GrantedTagsArray))
		{
			UTargetTagsGameplayEffectComponent& TagsComponent = EffectCDO->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
			FInheritedTagContainer TagContainer = TagsComponent.GetConfiguredTargetTagChanges();

			for (const TSharedPtr<FJsonValue>& TagValue : *GrantedTagsArray)
			{
				FString TagString;
				if (TagValue->TryGetString(TagString))
				{
					FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
					if (Tag.IsValid())
					{
						TagContainer.Added.AddTag(Tag);
					}
					else
					{
						return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid granted_tag: %s (tag not registered)"), *TagString));
					}
				}
			}

			TagsComponent.SetAndApplyTargetTagChanges(TagContainer);
		}

		// Process Application Tag Requirements
		const TArray<TSharedPtr<FJsonValue>>* AppRequiredTagsArray;
		const TArray<TSharedPtr<FJsonValue>>* AppBlockedTagsArray;
		bool bHasAppRequiredTags = Params->TryGetArrayField(TEXT("application_required_tags"), AppRequiredTagsArray);
		bool bHasAppBlockedTags = Params->TryGetArrayField(TEXT("application_blocked_tags"), AppBlockedTagsArray);

		if (bHasAppRequiredTags || bHasAppBlockedTags)
		{
			UTargetTagRequirementsGameplayEffectComponent& ReqComponent = EffectCDO->FindOrAddComponent<UTargetTagRequirementsGameplayEffectComponent>();

			if (bHasAppRequiredTags)
			{
				for (const TSharedPtr<FJsonValue>& TagValue : *AppRequiredTagsArray)
				{
					FString TagString;
					if (TagValue->TryGetString(TagString))
					{
						FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), true);
						if (Tag.IsValid())
						{
							ReqComponent.ApplicationTagRequirements.RequireTags.AddTag(Tag);
						}
						else
						{
							return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid application_required_tag: %s"), *TagString));
						}
					}
				}
			}

			if (bHasAppBlockedTags)
			{
				for (const TSharedPtr<FJsonValue>& TagValue : *AppBlockedTagsArray)
				{
					FString TagString;
					if (TagValue->TryGetString(TagString))
					{
						FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), true);
						if (Tag.IsValid())
						{
							ReqComponent.ApplicationTagRequirements.IgnoreTags.AddTag(Tag);
						}
						else
						{
							return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid application_blocked_tag: %s"), *TagString));
						}
					}
				}
			}
		}

		// Process Executions array (custom calculation classes)
		const TArray<TSharedPtr<FJsonValue>>* ExecutionsArray;
		if (Params->TryGetArrayField(TEXT("executions"), ExecutionsArray))
		{
			for (const TSharedPtr<FJsonValue>& ExecValue : *ExecutionsArray)
			{
				const TSharedPtr<FJsonObject>* ExecObj;
				if (ExecValue->TryGetObject(ExecObj))
				{
					FString CalculationClassName;
					(*ExecObj)->TryGetStringField(TEXT("calculation_class"), CalculationClassName);

					if (!CalculationClassName.IsEmpty())
					{
						// Try to load the calculation class
						UClass* CalcClass = LoadClass<UGameplayEffectExecutionCalculation>(nullptr, *CalculationClassName);

						if (!CalcClass)
						{
							// Try with /Script/GameplayAbilities. prefix
							FString FullPath = FString::Printf(TEXT("/Script/GameplayAbilities.%s"), *CalculationClassName);
							CalcClass = LoadClass<UGameplayEffectExecutionCalculation>(nullptr, *FullPath);
						}

						if (!CalcClass)
						{
							// Try searching in project
							for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
							{
								if (ClassIt->IsChildOf(UGameplayEffectExecutionCalculation::StaticClass()) &&
									!ClassIt->HasAnyClassFlags(CLASS_Abstract))
								{
									if (ClassIt->GetName() == CalculationClassName ||
										ClassIt->GetName().Contains(CalculationClassName))
									{
										CalcClass = *ClassIt;
										break;
									}
								}
							}
						}

						if (CalcClass)
						{
							FGameplayEffectExecutionDefinition ExecDef;
							ExecDef.CalculationClass = CalcClass;

							// Process conditional GameplayEffects to apply
							const TArray<TSharedPtr<FJsonValue>>* ConditionalEffectsArray;
							if ((*ExecObj)->TryGetArrayField(TEXT("conditional_effects"), ConditionalEffectsArray))
							{
								for (const TSharedPtr<FJsonValue>& CondEffectValue : *ConditionalEffectsArray)
								{
									const TSharedPtr<FJsonObject>* CondEffectObj;
									if (CondEffectValue->TryGetObject(CondEffectObj))
									{
										FString EffectClassPath;
										(*CondEffectObj)->TryGetStringField(TEXT("effect_class"), EffectClassPath);

										if (!EffectClassPath.IsEmpty())
										{
											// Load the conditional effect class
											UClass* CondEffectClass = nullptr;
											UBlueprint* EffectBlueprint = LoadObject<UBlueprint>(nullptr, *EffectClassPath);
											if (EffectBlueprint && EffectBlueprint->GeneratedClass)
											{
												CondEffectClass = EffectBlueprint->GeneratedClass;
											}

											if (CondEffectClass)
											{
												FConditionalGameplayEffect CondEffect;
												CondEffect.EffectClass = CondEffectClass;
												ExecDef.ConditionalGameplayEffects.Add(CondEffect);
											}
										}
									}
								}
							}

							// Process calculation modifiers (capture definitions)
							const TArray<TSharedPtr<FJsonValue>>* CalcModifiersArray;
							if ((*ExecObj)->TryGetArrayField(TEXT("calculation_modifiers"), CalcModifiersArray))
							{
								for (const TSharedPtr<FJsonValue>& CalcModValue : *CalcModifiersArray)
								{
									const TSharedPtr<FJsonObject>* CalcModObj;
									if (CalcModValue->TryGetObject(CalcModObj))
									{
										FString AttributeName;
										FString CaptureSource;
										FString SnapshotStr;

										(*CalcModObj)->TryGetStringField(TEXT("attribute"), AttributeName);
										(*CalcModObj)->TryGetStringField(TEXT("capture_source"), CaptureSource);
										(*CalcModObj)->TryGetStringField(TEXT("snapshot"), SnapshotStr);

										if (!AttributeName.IsEmpty())
										{
											FGameplayEffectAttributeCaptureDefinition CaptureDef;

											// Find the attribute
											for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
											{
												if (ClassIt->IsChildOf(UAttributeSet::StaticClass()) &&
													!ClassIt->HasAnyClassFlags(CLASS_Abstract))
												{
													for (TFieldIterator<FProperty> PropIt(*ClassIt); PropIt; ++PropIt)
													{
														if (PropIt->GetName() == AttributeName)
														{
															CaptureDef.AttributeToCapture = FGameplayAttribute(*PropIt);
															break;
														}
													}
													if (CaptureDef.AttributeToCapture.IsValid()) break;
												}
											}

											if (CaptureDef.AttributeToCapture.IsValid())
											{
												// Set capture source
												if (CaptureSource == TEXT("Target"))
												{
													CaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
												}
												else
												{
													CaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
												}

												// Set snapshot behavior
												CaptureDef.bSnapshot = (SnapshotStr == TEXT("true") || SnapshotStr == TEXT("True"));

												ExecDef.CalculationModifiers.Add(CaptureDef);
											}
										}
									}
								}
							}

							EffectCDO->Executions.Add(ExecDef);
							UE_LOG(LogTemp, Display, TEXT("FBlueprintCommands::HandleCreateGameplayEffect: Added Execution with CalculationClass: %s"), *CalcClass->GetName());
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("FBlueprintCommands::HandleCreateGameplayEffect: CalculationClass not found: %s"), *CalculationClassName);
						}
					}
				}
			}
		}

		// Mark as modified for save
		EffectCDO->MarkPackageDirty();
		NewBlueprint->MarkPackageDirty();
	}

	// Notify the asset registry
	FAssetRegistryModule::AssetCreated(NewBlueprint);

	// Save the package to disk
	FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPath + EffectName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	bool bSaved = UPackage::SavePackage(Package, NewBlueprint, *PackageFileName, SaveArgs);

	if (!bSaved)
	{
		UE_LOG(LogTemp, Warning, TEXT("FBlueprintCommands::HandleCreateGameplayEffect: Failed to save package to disk: %s"), *PackageFileName);
	}

	UE_LOG(LogTemp, Display, TEXT("FBlueprintCommands::HandleCreateGameplayEffect: Created '%s' (Parent: %s, Saved: %s)"),
		*EffectName, *ParentGEClass->GetName(), bSaved ? TEXT("Yes") : TEXT("No"));

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("name"), EffectName);
	ResultObj->SetStringField(TEXT("path"), AssetPath + EffectName);
	ResultObj->SetStringField(TEXT("full_path"), PackageFileName);
	ResultObj->SetStringField(TEXT("parent_class"), ParentGEClass->GetName());
	ResultObj->SetBoolField(TEXT("saved"), bSaved);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleCreateGameplayAbility(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString AbilityName;
	if (!Params->TryGetStringField(TEXT("name"), AbilityName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	// Get optional parameters
	FString AssetPath = TEXT("/Game/GAS/Abilities/");
	Params->TryGetStringField(TEXT("asset_path"), AssetPath);

	FString ParentClass = TEXT("GameplayAbility");
	Params->TryGetStringField(TEXT("parent_class"), ParentClass);

	bool bAutoSetupLifecycle = false;
	Params->TryGetBoolField(TEXT("auto_setup_lifecycle"), bAutoSetupLifecycle);

	// Tag configuration
	TArray<FString> AbilityTagStrings;
	TArray<FString> CancelTagStrings;
	TArray<FString> BlockTagStrings;

	const TArray<TSharedPtr<FJsonValue>>* AbilityTagsArray;
	if (Params->TryGetArrayField(TEXT("ability_tags"), AbilityTagsArray))
	{
		for (const TSharedPtr<FJsonValue>& TagValue : *AbilityTagsArray)
		{
			AbilityTagStrings.Add(TagValue->AsString());
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* CancelTagsArray;
	if (Params->TryGetArrayField(TEXT("cancel_abilities_with_tags"), CancelTagsArray))
	{
		for (const TSharedPtr<FJsonValue>& TagValue : *CancelTagsArray)
		{
			CancelTagStrings.Add(TagValue->AsString());
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* BlockTagsArray;
	if (Params->TryGetArrayField(TEXT("block_abilities_with_tags"), BlockTagsArray))
	{
		for (const TSharedPtr<FJsonValue>& TagValue : *BlockTagsArray)
		{
			BlockTagStrings.Add(TagValue->AsString());
		}
	}

	// Cost and Cooldown GameplayEffect references
	FString CostEffectPath;
	Params->TryGetStringField(TEXT("cost_gameplay_effect"), CostEffectPath);

	FString CooldownEffectPath;
	Params->TryGetStringField(TEXT("cooldown_gameplay_effect"), CooldownEffectPath);

	// Instancing and Network policies
	FString InstancingPolicyStr = TEXT("InstancedPerActor");
	Params->TryGetStringField(TEXT("instancing_policy"), InstancingPolicyStr);

	FString NetExecutionPolicyStr = TEXT("LocalPredicted");
	Params->TryGetStringField(TEXT("net_execution_policy"), NetExecutionPolicyStr);

	// Ensure path ends with /
	if (!AssetPath.EndsWith(TEXT("/")))
	{
		AssetPath += TEXT("/");
	}

	// Check if asset already exists
	if (UEditorAssetLibrary::DoesAssetExist(AssetPath + AbilityName))
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("GameplayAbility already exists: %s"), *AbilityName));
	}

	// Load parent class
	UClass* ParentGAClass = nullptr;

	if (ParentClass == TEXT("GameplayAbility") || ParentClass == TEXT("UGameplayAbility"))
	{
		ParentGAClass = UGameplayAbility::StaticClass();
	}
	else
	{
		// Load from path directly
		ParentGAClass = LoadClass<UGameplayAbility>(nullptr, *ParentClass);

		if (!ParentGAClass)
		{
			return FCommonUtils::CreateErrorResponse(FString::Printf(
				TEXT("Parent class not found: '%s'. Use search_assets to get the full class path."),
				*ParentClass));
		}
	}

	// Create the blueprint factory
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentGAClass;

	// Create the blueprint package
	FString FullAssetPath = AssetPath + AbilityName;
	UPackage* Package = CreatePackage(*FullAssetPath);
	UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(),
		Package,
		*AbilityName,
		RF_Standalone | RF_Public,
		nullptr,
		GWarn
	));

	if (!NewBlueprint)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create GameplayAbility Blueprint"));
	}

	// Get the CDO (Class Default Object) to set ability properties
	// UE 5.6+: Most properties are protected, use reflection to set them
	UGameplayAbility* AbilityCDO = Cast<UGameplayAbility>(NewBlueprint->GeneratedClass->GetDefaultObject());
	if (AbilityCDO)
	{
		UClass* AbilityClass = AbilityCDO->GetClass();

		// Helper lambda to set FGameplayTagContainer property via reflection
		auto SetTagContainerProperty = [&](const FName& PropertyName, const TArray<FString>& TagStrings) -> bool
		{
			if (TagStrings.Num() == 0) return true;

			FProperty* Prop = AbilityClass->FindPropertyByName(PropertyName);
			if (!Prop) return false;

			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (!StructProp) return false;

			FGameplayTagContainer* TagContainer = StructProp->ContainerPtrToValuePtr<FGameplayTagContainer>(AbilityCDO);
			if (!TagContainer) return false;

			for (const FString& TagStr : TagStrings)
			{
				FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagStr), false);
				if (Tag.IsValid())
				{
					TagContainer->AddTag(Tag);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("FBlueprintCommands::HandleCreateGameplayAbility: Invalid GameplayTag: %s (not registered)"), *TagStr);
				}
			}
			return true;
		};

		// Helper lambda to set TSubclassOf<UGameplayEffect> property via reflection
		auto SetEffectClassProperty = [&](const FName& PropertyName, const FString& EffectPath) -> bool
		{
			if (EffectPath.IsEmpty()) return true;

			UClass* EffectClass = nullptr;

			// GameplayEffect Blueprints are data-only, load as Blueprint and get GeneratedClass
			UBlueprint* EffectBlueprint = LoadObject<UBlueprint>(nullptr, *EffectPath);
			if (EffectBlueprint && EffectBlueprint->GeneratedClass)
			{
				EffectClass = EffectBlueprint->GeneratedClass;
			}

			if (!EffectClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("FBlueprintCommands::HandleCreateGameplayAbility: GameplayEffect Blueprint not found: %s"), *EffectPath);
				return false;
			}

			FProperty* Prop = AbilityClass->FindPropertyByName(PropertyName);
			if (!Prop) return false;

			FClassProperty* ClassProp = CastField<FClassProperty>(Prop);
			if (!ClassProp) return false;

			void* ValuePtr = ClassProp->ContainerPtrToValuePtr<void>(AbilityCDO);
			ClassProp->SetPropertyValue(ValuePtr, EffectClass);
			return true;
		};

		// Helper lambda to set enum property via reflection (for TEnumAsByte)
		auto SetByteEnumProperty = [&](const FName& PropertyName, uint8 EnumValue)
		{
			FProperty* Prop = AbilityClass->FindPropertyByName(PropertyName);
			if (!Prop) return;

			FByteProperty* ByteProp = CastField<FByteProperty>(Prop);
			if (ByteProp)
			{
				uint8* ValuePtr = ByteProp->ContainerPtrToValuePtr<uint8>(AbilityCDO);
				*ValuePtr = EnumValue;
			}
		};

		// Set AbilityTags (AbilityTags -> now using AssetTags internally, but property name may vary)
		SetTagContainerProperty(TEXT("AbilityTags"), AbilityTagStrings);

		// Set CancelAbilitiesWithTag
		SetTagContainerProperty(TEXT("CancelAbilitiesWithTag"), CancelTagStrings);

		// Set BlockAbilitiesWithTag
		SetTagContainerProperty(TEXT("BlockAbilitiesWithTag"), BlockTagStrings);

		// Set ActivationRequiredTags
		const TArray<TSharedPtr<FJsonValue>>* ActivationRequiredTagsArray;
		if (Params->TryGetArrayField(TEXT("activation_required_tags"), ActivationRequiredTagsArray))
		{
			TArray<FString> RequiredTagStrings;
			for (const TSharedPtr<FJsonValue>& TagValue : *ActivationRequiredTagsArray)
			{
				FString TagString;
				if (TagValue->TryGetString(TagString))
				{
					RequiredTagStrings.Add(TagString);
				}
			}
			SetTagContainerProperty(TEXT("ActivationRequiredTags"), RequiredTagStrings);
		}

		// Set ActivationBlockedTags
		const TArray<TSharedPtr<FJsonValue>>* ActivationBlockedTagsArray;
		if (Params->TryGetArrayField(TEXT("activation_blocked_tags"), ActivationBlockedTagsArray))
		{
			TArray<FString> BlockedTagStrings;
			for (const TSharedPtr<FJsonValue>& TagValue : *ActivationBlockedTagsArray)
			{
				FString TagString;
				if (TagValue->TryGetString(TagString))
				{
					BlockedTagStrings.Add(TagString);
				}
			}
			SetTagContainerProperty(TEXT("ActivationBlockedTags"), BlockedTagStrings);
		}

		// Set CostGameplayEffectClass
		SetEffectClassProperty(TEXT("CostGameplayEffectClass"), CostEffectPath);

		// Set CooldownGameplayEffectClass
		SetEffectClassProperty(TEXT("CooldownGameplayEffectClass"), CooldownEffectPath);

		// Set InstancingPolicy
		uint8 InstancingPolicyValue = static_cast<uint8>(EGameplayAbilityInstancingPolicy::InstancedPerActor);
		if (InstancingPolicyStr == TEXT("NonInstanced"))
		{
			InstancingPolicyValue = static_cast<uint8>(EGameplayAbilityInstancingPolicy::InstancedPerActor);
			UE_LOG(LogTemp, Warning, TEXT("FBlueprintCommands::HandleCreateGameplayAbility: NonInstanced is deprecated in UE 5.6+, using InstancedPerActor"));
		}
		else if (InstancingPolicyStr == TEXT("InstancedPerActor"))
		{
			InstancingPolicyValue = static_cast<uint8>(EGameplayAbilityInstancingPolicy::InstancedPerActor);
		}
		else if (InstancingPolicyStr == TEXT("InstancedPerExecution"))
		{
			InstancingPolicyValue = static_cast<uint8>(EGameplayAbilityInstancingPolicy::InstancedPerExecution);
		}
		SetByteEnumProperty(TEXT("InstancingPolicy"), InstancingPolicyValue);

		// Set NetExecutionPolicy
		uint8 NetExecutionPolicyValue = static_cast<uint8>(EGameplayAbilityNetExecutionPolicy::LocalPredicted);
		if (NetExecutionPolicyStr == TEXT("LocalOnly"))
		{
			NetExecutionPolicyValue = static_cast<uint8>(EGameplayAbilityNetExecutionPolicy::LocalOnly);
		}
		else if (NetExecutionPolicyStr == TEXT("LocalPredicted"))
		{
			NetExecutionPolicyValue = static_cast<uint8>(EGameplayAbilityNetExecutionPolicy::LocalPredicted);
		}
		else if (NetExecutionPolicyStr == TEXT("ServerOnly"))
		{
			NetExecutionPolicyValue = static_cast<uint8>(EGameplayAbilityNetExecutionPolicy::ServerOnly);
		}
		else if (NetExecutionPolicyStr == TEXT("ServerInitiated"))
		{
			NetExecutionPolicyValue = static_cast<uint8>(EGameplayAbilityNetExecutionPolicy::ServerInitiated);
		}
		SetByteEnumProperty(TEXT("NetExecutionPolicy"), NetExecutionPolicyValue);

		// Mark the CDO as modified
		AbilityCDO->MarkPackageDirty();
	}

	// Auto setup lifecycle nodes if requested
	// Creates: K2_ActivateAbility (override) -> CommitAbility -> Branch -> (True: user logic) / (False: EndAbility)
	TSharedPtr<FJsonObject> LifecycleNodes = MakeShared<FJsonObject>();
	if (bAutoSetupLifecycle)
	{
		// 1. Create K2_ActivateAbility function override (BlueprintImplementableEvent, DisplayName="ActivateAbility")
		UK2Node_FunctionEntry* ActivateEntry = nullptr;
		UEdGraph* ActivateGraph = FCommonUtils::CreateFunctionOverride(NewBlueprint, TEXT("K2_ActivateAbility"), ActivateEntry);

		if (ActivateGraph && ActivateEntry)
		{
			LifecycleNodes->SetStringField(TEXT("activate_ability_graph"), ActivateGraph->GetName());
			LifecycleNodes->SetStringField(TEXT("activate_ability_entry"), ActivateEntry->NodeGuid.ToString());

			// 2. Create CommitAbility function call node
			UFunction* CommitFunc = UGameplayAbility::StaticClass()->FindFunctionByName(TEXT("K2_CommitAbility"));
			if (!CommitFunc)
			{
				CommitFunc = UGameplayAbility::StaticClass()->FindFunctionByName(TEXT("CommitAbility"));
			}

			UK2Node_CallFunction* CommitNode = nullptr;
			if (CommitFunc)
			{
				CommitNode = FCommonUtils::CreateFunctionCallNode(ActivateGraph, CommitFunc, FVector2D(300, 0));
				if (CommitNode)
				{
					LifecycleNodes->SetStringField(TEXT("commit_ability"), CommitNode->NodeGuid.ToString());

					// Create Self node and connect to CommitAbility Target pin
					UK2Node_Self* SelfNode = FCommonUtils::CreateSelfReferenceNode(ActivateGraph, FVector2D(100, 100));
					if (SelfNode)
					{
						// Find Target pin on CommitNode and Self output pin
						UEdGraphPin* TargetPin = FCommonUtils::FindPin(CommitNode, TEXT("self"), EGPD_Input);
						if (!TargetPin)
						{
							TargetPin = FCommonUtils::FindPin(CommitNode, TEXT("Target"), EGPD_Input);
						}
						UEdGraphPin* SelfOutputPin = nullptr;
						for (UEdGraphPin* Pin : SelfNode->Pins)
						{
							if (Pin->Direction == EGPD_Output)
							{
								SelfOutputPin = Pin;
								break;
							}
						}
						if (TargetPin && SelfOutputPin)
						{
							SelfOutputPin->MakeLinkTo(TargetPin);
						}
					}

					// Connect ActivateAbility entry -> CommitAbility
					FCommonUtils::ConnectGraphNodes(ActivateGraph, ActivateEntry, TEXT("then"), CommitNode, TEXT("execute"));
				}
			}

			// 3. Create Branch node to check CommitAbility result
			UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(ActivateGraph);
			if (BranchNode)
			{
				BranchNode->CreateNewGuid();
				BranchNode->AllocateDefaultPins();
				BranchNode->NodePosX = 550;
				BranchNode->NodePosY = 0;
				ActivateGraph->AddNode(BranchNode, false, false);
				LifecycleNodes->SetStringField(TEXT("branch"), BranchNode->NodeGuid.ToString());

				// Connect CommitAbility ReturnValue -> Branch Condition
				if (CommitNode)
				{
					// Find CommitAbility's return value pin (bool)
					UEdGraphPin* CommitReturnPin = nullptr;
					for (UEdGraphPin* Pin : CommitNode->Pins)
					{
						if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
						{
							CommitReturnPin = Pin;
							break;
						}
					}

					// Find Branch's condition pin
					UEdGraphPin* BranchConditionPin = BranchNode->GetConditionPin();

					if (CommitReturnPin && BranchConditionPin)
					{
						CommitReturnPin->MakeLinkTo(BranchConditionPin);
					}

					// Connect CommitAbility exec -> Branch exec
					FCommonUtils::ConnectGraphNodes(ActivateGraph, CommitNode, TEXT("then"), BranchNode, TEXT("execute"));
				}
			}

			// 4. Create EndAbility function call node (connected to Branch False)
			UFunction* EndFunc = UGameplayAbility::StaticClass()->FindFunctionByName(TEXT("K2_EndAbility"));
			if (!EndFunc)
			{
				EndFunc = UGameplayAbility::StaticClass()->FindFunctionByName(TEXT("EndAbility"));
			}

			UK2Node_CallFunction* EndNode = nullptr;
			if (EndFunc)
			{
				EndNode = FCommonUtils::CreateFunctionCallNode(ActivateGraph, EndFunc, FVector2D(800, 150));
				if (EndNode)
				{
					LifecycleNodes->SetStringField(TEXT("end_ability"), EndNode->NodeGuid.ToString());

					// Connect Self to EndAbility Target pin
					UK2Node_Self* EndSelfNode = FCommonUtils::CreateSelfReferenceNode(ActivateGraph, FVector2D(600, 200));
					if (EndSelfNode)
					{
						UEdGraphPin* EndTargetPin = FCommonUtils::FindPin(EndNode, TEXT("self"), EGPD_Input);
						if (!EndTargetPin)
						{
							EndTargetPin = FCommonUtils::FindPin(EndNode, TEXT("Target"), EGPD_Input);
						}
						UEdGraphPin* EndSelfOutputPin = nullptr;
						for (UEdGraphPin* Pin : EndSelfNode->Pins)
						{
							if (Pin->Direction == EGPD_Output)
							{
								EndSelfOutputPin = Pin;
								break;
							}
						}
						if (EndTargetPin && EndSelfOutputPin)
						{
							EndSelfOutputPin->MakeLinkTo(EndTargetPin);
						}
					}

					// Connect Branch False -> EndAbility
					if (BranchNode)
					{
						UEdGraphPin* BranchFalsePin = BranchNode->GetElsePin();
						UEdGraphPin* EndExecPin = nullptr;
						for (UEdGraphPin* Pin : EndNode->Pins)
						{
							if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
							{
								EndExecPin = Pin;
								break;
							}
						}

						if (BranchFalsePin && EndExecPin)
						{
							BranchFalsePin->MakeLinkTo(EndExecPin);
						}
					}
				}
			}

			// 5. Create a second EndAbility node for True path (user connects their logic before this)
			if (EndFunc)
			{
				UK2Node_CallFunction* EndNodeTrue = FCommonUtils::CreateFunctionCallNode(ActivateGraph, EndFunc, FVector2D(1100, -50));
				if (EndNodeTrue)
				{
					LifecycleNodes->SetStringField(TEXT("end_ability_success"), EndNodeTrue->NodeGuid.ToString());

					// Connect Self to success EndAbility Target pin
					UK2Node_Self* EndTrueSelfNode = FCommonUtils::CreateSelfReferenceNode(ActivateGraph, FVector2D(900, 50));
					if (EndTrueSelfNode)
					{
						UEdGraphPin* EndTrueTargetPin = FCommonUtils::FindPin(EndNodeTrue, TEXT("self"), EGPD_Input);
						if (!EndTrueTargetPin)
						{
							EndTrueTargetPin = FCommonUtils::FindPin(EndNodeTrue, TEXT("Target"), EGPD_Input);
						}
						UEdGraphPin* EndTrueSelfOutputPin = nullptr;
						for (UEdGraphPin* Pin : EndTrueSelfNode->Pins)
						{
							if (Pin->Direction == EGPD_Output)
							{
								EndTrueSelfOutputPin = Pin;
								break;
							}
						}
						if (EndTrueTargetPin && EndTrueSelfOutputPin)
						{
							EndTrueSelfOutputPin->MakeLinkTo(EndTrueTargetPin);
						}
					}
				}
			}

			// Store Branch True pin info for user to connect their ability logic
			if (BranchNode)
			{
				LifecycleNodes->SetStringField(TEXT("branch_true_pin"), TEXT("Then"));
				LifecycleNodes->SetStringField(TEXT("usage_hint"), TEXT("Connect your ability logic between Branch(True) and EndAbility(Success)"));
			}

			// Mark blueprint as modified
			FBlueprintEditorUtils::MarkBlueprintAsModified(NewBlueprint);
		}
	}

	// Compile the blueprint
	FKismetEditorUtilities::CompileBlueprint(NewBlueprint);

	// Notify the asset registry
	FAssetRegistryModule::AssetCreated(NewBlueprint);
	Package->MarkPackageDirty();

	// Save the asset to disk
	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullAssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	bool bSaved = UPackage::SavePackage(Package, NewBlueprint, *PackageFileName, SaveArgs);

	// Prepare result object
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("name"), AbilityName);
	ResultObj->SetStringField(TEXT("path"), FullAssetPath);
	ResultObj->SetStringField(TEXT("full_path"), PackageFileName);
	ResultObj->SetStringField(TEXT("parent_class"), ParentGAClass->GetName());
	ResultObj->SetBoolField(TEXT("saved"), bSaved);

	// Add configuration info
	TSharedPtr<FJsonObject> ConfigObj = MakeShared<FJsonObject>();
	ConfigObj->SetStringField(TEXT("instancing_policy"), InstancingPolicyStr);
	ConfigObj->SetStringField(TEXT("net_execution_policy"), NetExecutionPolicyStr);

	// Add tags info
	TArray<TSharedPtr<FJsonValue>> AbilityTagsJson;
	for (const FString& TagStr : AbilityTagStrings)
	{
		AbilityTagsJson.Add(MakeShared<FJsonValueString>(TagStr));
	}
	ConfigObj->SetArrayField(TEXT("ability_tags"), AbilityTagsJson);

	TArray<TSharedPtr<FJsonValue>> CancelTagsJson;
	for (const FString& TagStr : CancelTagStrings)
	{
		CancelTagsJson.Add(MakeShared<FJsonValueString>(TagStr));
	}
	ConfigObj->SetArrayField(TEXT("cancel_abilities_with_tags"), CancelTagsJson);

	TArray<TSharedPtr<FJsonValue>> BlockTagsJson;
	for (const FString& TagStr : BlockTagStrings)
	{
		BlockTagsJson.Add(MakeShared<FJsonValueString>(TagStr));
	}
	ConfigObj->SetArrayField(TEXT("block_abilities_with_tags"), BlockTagsJson);

	if (!CostEffectPath.IsEmpty())
	{
		ConfigObj->SetStringField(TEXT("cost_gameplay_effect"), CostEffectPath);
	}
	if (!CooldownEffectPath.IsEmpty())
	{
		ConfigObj->SetStringField(TEXT("cooldown_gameplay_effect"), CooldownEffectPath);
	}

	ResultObj->SetObjectField(TEXT("configuration"), ConfigObj);

	// Add lifecycle nodes if created
	if (bAutoSetupLifecycle)
	{
		ResultObj->SetObjectField(TEXT("lifecycle_nodes"), LifecycleNodes);
	}

	UE_LOG(LogTemp, Display, TEXT("Created GameplayAbility Blueprint: %s (Parent: %s, Instancing: %s, Net: %s, Tags: %d, Saved: %s)"),
		*AbilityName, *ParentGAClass->GetName(), *InstancingPolicyStr, *NetExecutionPolicyStr,
		AbilityTagStrings.Num(), bSaved ? TEXT("Yes") : TEXT("No"));

	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

// ============================================================================
// GAS AttributeSet Commands
// ============================================================================

TSharedPtr<FJsonObject> FBlueprintCommands::HandleListAttributeSets(const TSharedPtr<FJsonObject>& Params)
{
	bool bIncludeEngine = false;
	Params->TryGetBoolField(TEXT("include_engine"), bIncludeEngine);

	int32 Limit = 50;
	if (Params->HasField(TEXT("limit")))
	{
		Limit = static_cast<int32>(Params->GetNumberField(TEXT("limit")));
	}

	TArray<TSharedPtr<FJsonValue>> AttributeSetsArray;
	int32 Count = 0;

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class || !Class->IsChildOf(UAttributeSet::StaticClass()))
		{
			continue;
		}

		// Skip abstract classes
		if (Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		// Skip engine classes if not requested
		if (!bIncludeEngine)
		{
			FString ClassPath = Class->GetPathName();
			if (ClassPath.StartsWith(TEXT("/Script/GameplayAbilities")) ||
				ClassPath.StartsWith(TEXT("/Script/Engine")))
			{
				continue;
			}
		}

		// Skip the base UAttributeSet class itself
		if (Class == UAttributeSet::StaticClass())
		{
			continue;
		}

		TSharedPtr<FJsonObject> SetInfo = MakeShared<FJsonObject>();
		SetInfo->SetStringField(TEXT("name"), Class->GetName());
		SetInfo->SetStringField(TEXT("path"), Class->GetPathName());

		if (UClass* ParentClass = Class->GetSuperClass())
		{
			SetInfo->SetStringField(TEXT("parent"), ParentClass->GetName());
		}

		// Count attributes
		int32 AttrCount = 0;
		for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
		{
			if (FGameplayAttribute::IsGameplayAttributeDataProperty(*PropIt))
			{
				AttrCount++;
			}
		}
		SetInfo->SetNumberField(TEXT("attribute_count"), AttrCount);

		AttributeSetsArray.Add(MakeShared<FJsonValueObject>(SetInfo));
		Count++;

		if (Count >= Limit)
		{
			break;
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetNumberField(TEXT("count"), AttributeSetsArray.Num());
	ResultObj->SetArrayField(TEXT("attribute_sets"), AttributeSetsArray);

	if (Count >= Limit)
	{
		ResultObj->SetBoolField(TEXT("truncated"), true);
	}

	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleGetAttributeSetInfo(const TSharedPtr<FJsonObject>& Params)
{
	FString AttributeSetName;
	if (!Params->TryGetStringField(TEXT("attribute_set_name"), AttributeSetName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'attribute_set_name' parameter"));
	}

	// Find the AttributeSet class
	UClass* FoundClass = nullptr;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class || !Class->IsChildOf(UAttributeSet::StaticClass()))
		{
			continue;
		}

		if (Class->GetName() == AttributeSetName ||
			Class->GetName().Contains(AttributeSetName, ESearchCase::IgnoreCase))
		{
			FoundClass = Class;
			break;
		}
	}

	if (!FoundClass)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("AttributeSet '%s' not found"), *AttributeSetName));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("name"), FoundClass->GetName());
	ResultObj->SetStringField(TEXT("path"), FoundClass->GetPathName());

	if (UClass* ParentClass = FoundClass->GetSuperClass())
	{
		ResultObj->SetStringField(TEXT("parent"), ParentClass->GetName());
	}

	// Collect attributes
	TArray<TSharedPtr<FJsonValue>> AttributesArray;
	for (TFieldIterator<FProperty> PropIt(FoundClass); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		if (!FGameplayAttribute::IsGameplayAttributeDataProperty(Property))
		{
			continue;
		}

		TSharedPtr<FJsonObject> AttrInfo = MakeShared<FJsonObject>();
		AttrInfo->SetStringField(TEXT("name"), Property->GetName());
		AttrInfo->SetStringField(TEXT("type"), Property->GetCPPType());

		// Check if inherited
		bool bInherited = Property->GetOwnerClass() != FoundClass;
		AttrInfo->SetBoolField(TEXT("inherited"), bInherited);
		if (bInherited && Property->GetOwnerClass())
		{
			AttrInfo->SetStringField(TEXT("defined_in"), Property->GetOwnerClass()->GetName());
		}

		AttributesArray.Add(MakeShared<FJsonValueObject>(AttrInfo));
	}

	ResultObj->SetNumberField(TEXT("attribute_count"), AttributesArray.Num());
	ResultObj->SetArrayField(TEXT("attributes"), AttributesArray);

	return ResultObj;
}

// ============================================================================
// Tier 1: Core Blueprint Node Tools
// ============================================================================

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddBlueprintFlowControlNode(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString ControlType;
	if (!Params->TryGetStringField(TEXT("control_type"), ControlType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'control_type' parameter (branch, sequence)"));
	}

	// Get optional parameters
	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	// Get target graph
	UEdGraph* EventGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		EventGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	}
	if (!EventGraph)
	{
		EventGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	}
	if (!EventGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get target graph"));
	}

	UEdGraphNode* NewNode = nullptr;
	TArray<FString> OutputPinNames;

	if (ControlType.ToLower() == TEXT("branch"))
	{
		UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(EventGraph);
		if (BranchNode)
		{
			BranchNode->CreateNewGuid();
			BranchNode->NodePosX = NodePosition.X;
			BranchNode->NodePosY = NodePosition.Y;
			EventGraph->AddNode(BranchNode, true);
			BranchNode->PostPlacedNewNode();
			BranchNode->AllocateDefaultPins();
			NewNode = BranchNode;
			OutputPinNames.Add(TEXT("True"));
			OutputPinNames.Add(TEXT("False"));
		}
	}
	else if (ControlType.ToLower() == TEXT("sequence"))
	{
		UK2Node_ExecutionSequence* SeqNode = NewObject<UK2Node_ExecutionSequence>(EventGraph);
		if (SeqNode)
		{
			SeqNode->CreateNewGuid();
			SeqNode->NodePosX = NodePosition.X;
			SeqNode->NodePosY = NodePosition.Y;
			EventGraph->AddNode(SeqNode, true);
			SeqNode->PostPlacedNewNode();
			SeqNode->AllocateDefaultPins();
			NewNode = SeqNode;

			for (UEdGraphPin* Pin : SeqNode->Pins)
			{
				if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
				{
					OutputPinNames.Add(Pin->PinName.ToString());
				}
			}
		}
	}
	else
	{
		// Macro-based flow control nodes
		FString MacroPath;
		FString ControlTypeLower = ControlType.ToLower();

		if (ControlTypeLower == TEXT("forloop") || ControlTypeLower == TEXT("for"))
		{
			MacroPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:ForLoop");
		}
		else if (ControlTypeLower == TEXT("foreachloop") || ControlTypeLower == TEXT("foreach"))
		{
			MacroPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:ForEachLoop");
		}
		else if (ControlTypeLower == TEXT("foreachloopwithbreak"))
		{
			MacroPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:ForEachLoopWithBreak");
		}
		else if (ControlTypeLower == TEXT("whileloop") || ControlTypeLower == TEXT("while"))
		{
			MacroPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:WhileLoop");
		}
		else if (ControlTypeLower == TEXT("doonce"))
		{
			MacroPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:DoOnce");
		}
		else if (ControlTypeLower == TEXT("donmultigate") || ControlTypeLower == TEXT("multigate"))
		{
			MacroPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:DoN");
		}
		else if (ControlTypeLower == TEXT("flipflop"))
		{
			MacroPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:FlipFlop");
		}
		else if (ControlTypeLower == TEXT("gate"))
		{
			MacroPath = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:Gate");
		}

		if (!MacroPath.IsEmpty())
		{
			UEdGraph* MacroGraph = LoadObject<UEdGraph>(nullptr, *MacroPath);
			if (MacroGraph)
			{
				UK2Node_MacroInstance* MacroNode = NewObject<UK2Node_MacroInstance>(EventGraph);
				if (MacroNode)
				{
					MacroNode->CreateNewGuid();
					MacroNode->SetMacroGraph(MacroGraph);
					MacroNode->NodePosX = NodePosition.X;
					MacroNode->NodePosY = NodePosition.Y;
					EventGraph->AddNode(MacroNode, true);
					MacroNode->PostPlacedNewNode();
					MacroNode->AllocateDefaultPins();
					NewNode = MacroNode;

					for (UEdGraphPin* Pin : MacroNode->Pins)
					{
						if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
						{
							OutputPinNames.Add(Pin->PinName.ToString());
						}
					}
				}
			}
			else
			{
				return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load macro: %s"), *MacroPath));
			}
		}
		else
		{
			return FCommonUtils::CreateErrorResponse(FString::Printf(
				TEXT("Unknown control_type: %s. Supported: branch, sequence, forloop, foreachloop, foreachloopwithbreak, whileloop, doonce, multigate, flipflop, gate"),
				*ControlType));
		}
	}

	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create %s node"), *ControlType));
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = FCommonUtils::CreateNodeResponse(NewNode);
	ResultObj->SetStringField(TEXT("control_type"), ControlType);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleSetPinDefaultValue(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
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

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Search all graphs for the node
	UEdGraphNode* TargetNode = FCommonUtils::FindNodeByGuidInBlueprint(Blueprint, NodeId);
	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	// Find the pin (try input first, then output for flexibility)
	UEdGraphPin* TargetPin = FCommonUtils::FindPin(TargetNode, PinName, EGPD_Input);
	if (!TargetPin)
	{
		TargetPin = FCommonUtils::FindPin(TargetNode, PinName, EGPD_Output);
	}
	if (!TargetPin)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Pin not found: %s"), *PinName));
	}

	// Store original value for verification
	FString OriginalValue = TargetPin->DefaultValue;
	UObject* OriginalObject = TargetPin->DefaultObject;

	FString ValueStr;
	double ValueNum = 0;
	bool ValueBool = false;
	const FName& PinCategory = TargetPin->PinType.PinCategory;

	// Wildcard pins require type resolution first
	if (PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Pin '%s' is Wildcard type. Connect a typed pin first to resolve the type."), *PinName));
	}

	// Handle bool type explicitly (JSON bool or string "true"/"false")
	if (PinCategory == UEdGraphSchema_K2::PC_Boolean)
	{
		if (Params->TryGetBoolField(TEXT("value"), ValueBool))
		{
			TargetPin->DefaultValue = ValueBool ? TEXT("true") : TEXT("false");
		}
		else if (Params->TryGetStringField(TEXT("value"), ValueStr))
		{
			FString LowerValue = ValueStr.ToLower();
			if (LowerValue == TEXT("true") || LowerValue == TEXT("1"))
			{
				TargetPin->DefaultValue = TEXT("true");
			}
			else if (LowerValue == TEXT("false") || LowerValue == TEXT("0"))
			{
				TargetPin->DefaultValue = TEXT("false");
			}
			else
			{
				return FCommonUtils::CreateErrorResponse(FString::Printf(
					TEXT("Invalid boolean value: %s"), *ValueStr));
			}
		}
		else
		{
			return FCommonUtils::CreateErrorResponse(TEXT("Missing or invalid 'value' for boolean pin"));
		}
	}
	else if (Params->TryGetStringField(TEXT("value"), ValueStr))
	{
		if (PinCategory == UEdGraphSchema_K2::PC_Class || PinCategory == UEdGraphSchema_K2::PC_SoftClass)
		{
			UClass* FoundClass = nullptr;

			if (ValueStr.Contains(TEXT("/")))
			{
				UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *ValueStr);
				if (BP && BP->GeneratedClass)
				{
					FoundClass = BP->GeneratedClass;
				}
				else
				{
					FoundClass = LoadClass<UObject>(nullptr, *ValueStr);
				}
			}
			else
			{
				FoundClass = FCommonUtils::FindClassByName(ValueStr);
			}

			if (FoundClass)
			{
				TargetPin->DefaultObject = FoundClass;
				TargetPin->DefaultValue = FoundClass->GetPathName();
			}
			else
			{
				return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Class not found: %s"), *ValueStr));
			}
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Object || PinCategory == UEdGraphSchema_K2::PC_SoftObject)
		{
			UObject* FoundObject = LoadObject<UObject>(nullptr, *ValueStr);
			if (FoundObject)
			{
				TargetPin->DefaultObject = FoundObject;
				TargetPin->DefaultValue = FoundObject->GetPathName();
			}
			else
			{
				return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Object not found: %s"), *ValueStr));
			}
		}
		else if (PinCategory == UEdGraphSchema_K2::PC_Byte)
		{
			// Handle Enum types (PC_Byte with PinSubCategoryObject as UEnum)
			UEnum* EnumType = Cast<UEnum>(TargetPin->PinType.PinSubCategoryObject.Get());
			if (EnumType)
			{
				// Try direct value first
				int64 EnumValue = EnumType->GetValueByNameString(ValueStr);
				if (EnumValue == INDEX_NONE)
				{
					// Try with enum prefix (e.g., "ObjectTypeQuery3" -> "EObjectTypeQuery::ObjectTypeQuery3")
					FString FullEnumName = FString::Printf(TEXT("%s::%s"), *EnumType->GetName(), *ValueStr);
					EnumValue = EnumType->GetValueByNameString(FullEnumName);
				}
				if (EnumValue != INDEX_NONE)
				{
					TargetPin->DefaultValue = EnumType->GetNameStringByValue(EnumValue);
				}
				else
				{
					TargetPin->DefaultValue = ValueStr;
				}
			}
			else
			{
				TargetPin->DefaultValue = ValueStr;
			}
		}
		else
		{
			TargetPin->DefaultValue = ValueStr;
		}
	}
	else if (Params->TryGetNumberField(TEXT("value"), ValueNum))
	{
		if (PinCategory == UEdGraphSchema_K2::PC_Int)
		{
			TargetPin->DefaultValue = FString::FromInt(FMath::RoundToInt(ValueNum));
		}
		else
		{
			TargetPin->DefaultValue = FString::SanitizeFloat(ValueNum);
		}
	}
	else if (Params->TryGetBoolField(TEXT("value"), ValueBool))
	{
		TargetPin->DefaultValue = ValueBool ? TEXT("true") : TEXT("false");
	}
	else if (Params->HasField(TEXT("value")))
	{
		const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
		if (Params->TryGetArrayField(TEXT("value"), ArrayValue))
		{
			// Detect struct type from pin for accurate formatting
			UScriptStruct* StructType = Cast<UScriptStruct>(TargetPin->PinType.PinSubCategoryObject.Get());

			if (ArrayValue->Num() == 2)
			{
				float X = (*ArrayValue)[0]->AsNumber();
				float Y = (*ArrayValue)[1]->AsNumber();
				TargetPin->DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f)"), X, Y);
			}
			else if (ArrayValue->Num() == 3)
			{
				float A = (*ArrayValue)[0]->AsNumber();
				float B = (*ArrayValue)[1]->AsNumber();
				float C = (*ArrayValue)[2]->AsNumber();

				if (StructType == TBaseStructure<FRotator>::Get())
				{
					TargetPin->DefaultValue = FString::Printf(TEXT("(Pitch=%f,Yaw=%f,Roll=%f)"), A, B, C);
				}
				else
				{
					TargetPin->DefaultValue = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), A, B, C);
				}
			}
			else if (ArrayValue->Num() == 4)
			{
				float A = (*ArrayValue)[0]->AsNumber();
				float B = (*ArrayValue)[1]->AsNumber();
				float C = (*ArrayValue)[2]->AsNumber();
				float D = (*ArrayValue)[3]->AsNumber();
				TargetPin->DefaultValue = FString::Printf(TEXT("(R=%f,G=%f,B=%f,A=%f)"), A, B, C, D);
			}
			else
			{
				return FCommonUtils::CreateErrorResponse(TEXT("Unsupported array size for value"));
			}
		}
		else
		{
			return FCommonUtils::CreateErrorResponse(TEXT("Unsupported value type"));
		}
	}
	else if (!Params->HasField(TEXT("value")))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	// Verify value was actually changed
	bool bValueChanged = (TargetPin->DefaultValue != OriginalValue) || (TargetPin->DefaultObject != OriginalObject);

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), NodeId);
	ResultObj->SetStringField(TEXT("pin_name"), PinName);
	ResultObj->SetStringField(TEXT("pin_type"), PinCategory.ToString());
	ResultObj->SetStringField(TEXT("value_set"), TargetPin->DefaultValue);
	if (TargetPin->DefaultObject)
	{
		ResultObj->SetStringField(TEXT("object_set"), TargetPin->DefaultObject->GetPathName());
	}
	ResultObj->SetBoolField(TEXT("value_changed"), bValueChanged);
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleGetPinValue(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
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

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraphNode* TargetNode = FCommonUtils::FindNodeByGuidInBlueprint(Blueprint, NodeId);
	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	UEdGraphPin* TargetPin = FCommonUtils::FindPin(TargetNode, PinName, EGPD_Input);
	if (!TargetPin)
	{
		TargetPin = FCommonUtils::FindPin(TargetNode, PinName, EGPD_Output);
	}
	if (!TargetPin)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Pin not found: %s"), *PinName));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("node_id"), NodeId);
	ResultObj->SetStringField(TEXT("pin_name"), PinName);
	ResultObj->SetStringField(TEXT("pin_type"), TargetPin->PinType.PinCategory.ToString());
	ResultObj->SetStringField(TEXT("default_value"), TargetPin->DefaultValue);
	if (TargetPin->DefaultObject)
	{
		ResultObj->SetStringField(TEXT("default_object"), TargetPin->DefaultObject->GetPathName());
	}
	ResultObj->SetBoolField(TEXT("has_connection"), TargetPin->HasAnyConnections());
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddBlueprintVariableNode(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	FString NodeType;
	if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'node_type' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	// Find the blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Get target graph
	UEdGraph* EventGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		EventGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	}
	if (!EventGraph)
	{
		EventGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	}
	if (!EventGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get target graph"));
	}

	UEdGraphNode* NewNode = nullptr;

	if (NodeType.ToLower() == TEXT("get"))
	{
		// Create Variable Get node
		UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(EventGraph);
		if (GetNode)
		{
			GetNode->CreateNewGuid();

			FMemberReference& VarRef = GetNode->VariableReference;
			VarRef.SetSelfMember(FName(*VariableName));

			GetNode->NodePosX = NodePosition.X;
			GetNode->NodePosY = NodePosition.Y;

			EventGraph->AddNode(GetNode, true);
			GetNode->PostPlacedNewNode();
			GetNode->AllocateDefaultPins();
			GetNode->ReconstructNode();

			NewNode = GetNode;
		}
	}
	else if (NodeType.ToLower() == TEXT("set"))
	{
		UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(EventGraph);
		if (SetNode)
		{
			SetNode->CreateNewGuid();

			FMemberReference& VarRef = SetNode->VariableReference;
			VarRef.SetSelfMember(FName(*VariableName));

			SetNode->NodePosX = NodePosition.X;
			SetNode->NodePosY = NodePosition.Y;

			EventGraph->AddNode(SetNode, true);
			SetNode->PostPlacedNewNode();
			SetNode->AllocateDefaultPins();
			SetNode->ReconstructNode();

			NewNode = SetNode;
		}
	}
	else
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown node_type: %s. Supported: get, set"), *NodeType));
	}

	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create variable node"));
	}

	// Mark the blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = FCommonUtils::CreateNodeResponse(NewNode);
	ResultObj->SetStringField(TEXT("variable_name"), VariableName);
	ResultObj->SetStringField(TEXT("node_type"), NodeType);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleSearchFunctions(const TSharedPtr<FJsonObject>& Params)
{
	FString Keyword;
	if (!Params->TryGetStringField(TEXT("keyword"), Keyword))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'keyword' parameter"));
	}

	FString ClassFilter;
	Params->TryGetStringField(TEXT("class_filter"), ClassFilter);

	int32 MaxResults = 20;
	Params->TryGetNumberField(TEXT("max_results"), MaxResults);
	MaxResults = FMath::Clamp(MaxResults, 1, 100);

	TArray<TSharedPtr<FJsonValue>> Results;

	auto ProcessClass = [&](UClass* Class)
	{
		if (!Class || Results.Num() >= MaxResults) return;

		for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
		{
			if (Results.Num() >= MaxResults) break;

			UFunction* Func = *FuncIt;
			if (!Func->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure)) continue;

			FString FuncName = Func->GetName();
			if (!FuncName.Contains(Keyword, ESearchCase::IgnoreCase)) continue;

			TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
			FuncObj->SetStringField(TEXT("class"), Class->GetName());
			FuncObj->SetStringField(TEXT("function"), FuncName);

			// Build signature
			FString Signature = TEXT("(");
			bool bFirst = true;
			for (TFieldIterator<FProperty> PropIt(Func); PropIt; ++PropIt)
			{
				FProperty* Prop = *PropIt;
				if (Prop->HasAnyPropertyFlags(CPF_Parm) && !Prop->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					if (!bFirst) Signature += TEXT(", ");
					Signature += Prop->GetCPPType() + TEXT(" ") + Prop->GetName();
					bFirst = false;
				}
			}
			Signature += TEXT(")");

			// Return type
			if (FProperty* ReturnProp = Func->GetReturnProperty())
			{
				Signature += TEXT(" -> ") + ReturnProp->GetCPPType();
			}
			FuncObj->SetStringField(TEXT("signature"), Signature);

			// Tooltip
			FString Tooltip = Func->GetMetaData(TEXT("Tooltip"));
			if (!Tooltip.IsEmpty())
			{
				FuncObj->SetStringField(TEXT("tooltip"), Tooltip);
			}

			Results.Add(MakeShared<FJsonValueObject>(FuncObj));
		}
	};

	if (!ClassFilter.IsEmpty())
	{
		UClass* TargetClass = FCommonUtils::FindClassByName(ClassFilter);
		if (TargetClass)
		{
			ProcessClass(TargetClass);
		}
	}
	else
	{
		// Search common classes
		TArray<UClass*> ClassesToSearch = {
			AActor::StaticClass(),
			APawn::StaticClass(),
			ACharacter::StaticClass(),
			UGameplayStatics::StaticClass(),
			UKismetMathLibrary::StaticClass(),
			UAbilitySystemComponent::StaticClass(),
			UGameplayAbility::StaticClass(),
			UAbilitySystemBlueprintLibrary::StaticClass()
		};

		for (UClass* Class : ClassesToSearch)
		{
			ProcessClass(Class);
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetArrayField(TEXT("functions"), Results);
	ResultObj->SetNumberField(TEXT("count"), Results.Num());
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleGetClassFunctions(const TSharedPtr<FJsonObject>& Params)
{
	FString ClassName;
	if (!Params->TryGetStringField(TEXT("class_name"), ClassName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'class_name' parameter"));
	}

	bool bIncludeInherited = false;
	Params->TryGetBoolField(TEXT("include_inherited"), bIncludeInherited);

	bool bCallableOnly = true;
	Params->TryGetBoolField(TEXT("callable_only"), bCallableOnly);

	UClass* TargetClass = FCommonUtils::FindClassByName(ClassName);
	if (!TargetClass)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Class not found: %s"), *ClassName));
	}

	TArray<TSharedPtr<FJsonValue>> Functions;

	EFieldIteratorFlags::SuperClassFlags SuperFlag = bIncludeInherited
		? EFieldIteratorFlags::IncludeSuper
		: EFieldIteratorFlags::ExcludeSuper;

	for (TFieldIterator<UFunction> FuncIt(TargetClass, SuperFlag); FuncIt; ++FuncIt)
	{
		UFunction* Func = *FuncIt;

		if (bCallableOnly && !Func->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure))
		{
			continue;
		}

		TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
		FuncObj->SetStringField(TEXT("name"), Func->GetName());

		// Flags
		TArray<FString> Flags;
		if (Func->HasAnyFunctionFlags(FUNC_BlueprintPure)) Flags.Add(TEXT("Pure"));
		if (Func->HasAnyFunctionFlags(FUNC_BlueprintCallable)) Flags.Add(TEXT("Callable"));
		if (Func->HasAnyFunctionFlags(FUNC_Static)) Flags.Add(TEXT("Static"));

		TArray<TSharedPtr<FJsonValue>> FlagArray;
		for (const FString& Flag : Flags)
		{
			FlagArray.Add(MakeShared<FJsonValueString>(Flag));
		}
		FuncObj->SetArrayField(TEXT("flags"), FlagArray);

		// Parameters
		TArray<TSharedPtr<FJsonValue>> ParamArray;
		for (TFieldIterator<FProperty> PropIt(Func); PropIt; ++PropIt)
		{
			FProperty* Prop = *PropIt;
			if (Prop->HasAnyPropertyFlags(CPF_Parm) && !Prop->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
				ParamObj->SetStringField(TEXT("name"), Prop->GetName());
				ParamObj->SetStringField(TEXT("type"), Prop->GetCPPType());
				ParamArray.Add(MakeShared<FJsonValueObject>(ParamObj));
			}
		}
		FuncObj->SetArrayField(TEXT("params"), ParamArray);

		// Return type
		if (FProperty* ReturnProp = Func->GetReturnProperty())
		{
			FuncObj->SetStringField(TEXT("return_type"), ReturnProp->GetCPPType());
		}

		// Tooltip
		FString Tooltip = Func->GetMetaData(TEXT("Tooltip"));
		if (!Tooltip.IsEmpty())
		{
			FuncObj->SetStringField(TEXT("tooltip"), Tooltip);
		}

		Functions.Add(MakeShared<FJsonValueObject>(FuncObj));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("class"), TargetClass->GetName());
	ResultObj->SetArrayField(TEXT("functions"), Functions);
	ResultObj->SetNumberField(TEXT("count"), Functions.Num());
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddFunctionOverride(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UK2Node_FunctionEntry* FunctionEntry = nullptr;
	UEdGraph* OverrideGraph = FCommonUtils::CreateFunctionOverride(Blueprint, FunctionName, FunctionEntry);

	if (!OverrideGraph || !FunctionEntry)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create override for function: %s"), *FunctionName));
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
	ResultObj->SetStringField(TEXT("function_name"), FunctionName);
	ResultObj->SetStringField(TEXT("graph_name"), OverrideGraph->GetName());
	ResultObj->SetStringField(TEXT("entry_node_id"), FunctionEntry->NodeGuid.ToString());
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddAbilityTaskNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString TaskClassName;
	if (!Params->TryGetStringField(TEXT("task_class"), TaskClassName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'task_class' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter (e.g. 'CreatePlayMontageAndWaitProxy')"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* TargetGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		TargetGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	}
	if (!TargetGraph)
	{
		TargetGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	}
	if (!TargetGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get target graph"));
	}

	UClass* TaskClass = FCommonUtils::FindClassByName(TaskClassName);
	if (!TaskClass)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Task class not found: %s"), *TaskClassName));
	}

	UFunction* ProxyFunction = TaskClass->FindFunctionByName(*FunctionName);
	if (!ProxyFunction)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s::%s"), *TaskClassName, *FunctionName));
	}

	UK2Node_CallFunction* TaskNode = NewObject<UK2Node_CallFunction>(TargetGraph);
	if (!TaskNode)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create CallFunction node"));
	}

	TaskNode->CreateNewGuid();
	TaskNode->SetFromFunction(ProxyFunction);
	TaskNode->NodePosX = NodePosition.X;
	TaskNode->NodePosY = NodePosition.Y;
	TargetGraph->AddNode(TaskNode, true);
	TaskNode->PostPlacedNewNode();
	TaskNode->AllocateDefaultPins();

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = FCommonUtils::CreateNodeResponse(TaskNode);
	ResultObj->SetStringField(TEXT("task_class"), TaskClassName);
	ResultObj->SetStringField(TEXT("function_name"), FunctionName);
	return ResultObj;
}

// ============================================================================
// Generic Node Tools
// ============================================================================

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddGenericNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString NodeClassName;
	if (!Params->TryGetStringField(TEXT("node_class"), NodeClassName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'node_class' parameter"));
	}

	FVector2D NodePosition(0.0f, 0.0f);
	if (Params->HasField(TEXT("node_position")))
	{
		NodePosition = FCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString GraphName = TEXT("EventGraph");
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* TargetGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!TargetGraph)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	UEdGraphNode* NewNode = FCommonUtils::CreateNodeByClassName(TargetGraph, NodeClassName, NodePosition);
	if (!NewNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create node: %s"), *NodeClassName));
	}

	// Post-creation property setup
	bool bNeedsReconstruct = false;

	if (NodeClassName.Contains(TEXT("MakeStruct")))
	{
		UK2Node_MakeStruct* MakeNode = Cast<UK2Node_MakeStruct>(NewNode);
		if (MakeNode)
		{
			FString StructPath;
			if (Params->TryGetStringField(TEXT("StructType"), StructPath) ||
			    Params->TryGetStringField(TEXT("struct_type"), StructPath))
			{
				UScriptStruct* Struct = LoadObject<UScriptStruct>(nullptr, *StructPath);
				if (Struct && MakeNode->StructType != Struct)
				{
					MakeNode->StructType = Struct;
					bNeedsReconstruct = true;
				}
			}
		}
	}
	else if (NodeClassName.Contains(TEXT("BreakStruct")))
	{
		UK2Node_BreakStruct* BreakNode = Cast<UK2Node_BreakStruct>(NewNode);
		if (BreakNode)
		{
			FString StructPath;
			if (Params->TryGetStringField(TEXT("StructType"), StructPath) ||
			    Params->TryGetStringField(TEXT("struct_type"), StructPath))
			{
				UScriptStruct* Struct = LoadObject<UScriptStruct>(nullptr, *StructPath);
				if (Struct && BreakNode->StructType != Struct)
				{
					BreakNode->StructType = Struct;
					bNeedsReconstruct = true;
				}
			}
		}
	}
	else if (NodeClassName.Contains(TEXT("SwitchEnum")))
	{
		UK2Node_SwitchEnum* SwitchNode = Cast<UK2Node_SwitchEnum>(NewNode);
		if (SwitchNode)
		{
			FString EnumPath;
			if (Params->TryGetStringField(TEXT("Enum"), EnumPath) ||
			    Params->TryGetStringField(TEXT("enum"), EnumPath))
			{
				UEnum* EnumType = LoadObject<UEnum>(nullptr, *EnumPath);
				if (EnumType)
				{
					SwitchNode->SetEnum(EnumType);
					bNeedsReconstruct = true;
				}
			}
		}
	}
	else if (NodeClassName.Contains(TEXT("DynamicCast")))
	{
		UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(NewNode);
		if (CastNode)
		{
			FString TargetTypePath;
			if (Params->TryGetStringField(TEXT("TargetType"), TargetTypePath) ||
			    Params->TryGetStringField(TEXT("target_type"), TargetTypePath))
			{
				UClass* TargetType = LoadClass<UObject>(nullptr, *TargetTypePath);
				if (TargetType && CastNode->TargetType != TargetType)
				{
					CastNode->TargetType = TargetType;
					bNeedsReconstruct = true;
				}
			}
		}
	}
	else if (NodeClassName.Contains(TEXT("SpawnActorFromClass")))
	{
		UK2Node_SpawnActorFromClass* SpawnNode = Cast<UK2Node_SpawnActorFromClass>(NewNode);
		if (SpawnNode)
		{
			FString ClassPath;
			if (Params->TryGetStringField(TEXT("ActorClass"), ClassPath) ||
			    Params->TryGetStringField(TEXT("actor_class"), ClassPath))
			{
				UClass* ActorClass = LoadClass<AActor>(nullptr, *ClassPath);
				if (!ActorClass)
				{
					ActorClass = FCommonUtils::FindClassByName(ClassPath);
				}
				if (ActorClass && ActorClass->IsChildOf(AActor::StaticClass()))
				{
					UEdGraphPin* ClassPin = SpawnNode->GetClassPin();
					if (ClassPin)
					{
						ClassPin->DefaultObject = ActorClass;
						SpawnNode->PinDefaultValueChanged(ClassPin);
					}
					bNeedsReconstruct = true;
				}
			}
		}
	}
	else if (NodeClassName.Contains(TEXT("ConstructObjectFromClass")))
	{
		UK2Node_ConstructObjectFromClass* ConstructNode = Cast<UK2Node_ConstructObjectFromClass>(NewNode);
		if (ConstructNode)
		{
			FString ClassPath;
			if (Params->TryGetStringField(TEXT("ObjectClass"), ClassPath) ||
			    Params->TryGetStringField(TEXT("object_class"), ClassPath))
			{
				UClass* ObjectClass = LoadClass<UObject>(nullptr, *ClassPath);
				if (!ObjectClass)
				{
					ObjectClass = FCommonUtils::FindClassByName(ClassPath);
				}
				if (ObjectClass)
				{
					UEdGraphPin* ClassPin = ConstructNode->GetClassPin();
					if (ClassPin)
					{
						ClassPin->DefaultObject = ObjectClass;
						ConstructNode->PinDefaultValueChanged(ClassPin);
					}
					bNeedsReconstruct = true;
				}
			}
		}
	}

	if (bNeedsReconstruct)
	{
		NewNode->ReconstructNode();
	}

	FCommonUtils::InitializeNodeFromParams(NewNode, Params);

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = FCommonUtils::CreateNodeResponse(NewNode);
	ResultObj->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleSetNodeProperty(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	FString PropertyPath;
	if (!Params->TryGetStringField(TEXT("property_path"), PropertyPath))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'property_path' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraphNode* TargetNode = FCommonUtils::FindNodeByGuidInBlueprint(Blueprint, NodeId);
	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	TSharedPtr<FJsonValue> Value = Params->TryGetField(TEXT("value"));
	if (!Value.IsValid())
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	FString OutError;
	if (!FCommonUtils::SetNodePropertyByPath(TargetNode, PropertyPath, Value, OutError))
	{
		return FCommonUtils::CreateErrorResponse(OutError);
	}

	TargetNode->ReconstructNode();
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("node_id"), NodeId);
	ResultObj->SetStringField(TEXT("property_path"), PropertyPath);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleConnectNodes(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
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

	bool bConnectExec = true;
	bool bConnectData = false;
	Params->TryGetBoolField(TEXT("connect_exec"), bConnectExec);
	Params->TryGetBoolField(TEXT("connect_data"), bConnectData);

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraphNode* SourceNode = FCommonUtils::FindNodeByGuidInBlueprint(Blueprint, SourceNodeId);
	if (!SourceNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Source node not found: %s"), *SourceNodeId));
	}

	UEdGraphNode* TargetNode = FCommonUtils::FindNodeByGuidInBlueprint(Blueprint, TargetNodeId);
	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Target node not found: %s"), *TargetNodeId));
	}

	UEdGraph* Graph = SourceNode->GetGraph();
	if (!Graph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get graph from source node"));
	}

	bool bConnected = FCommonUtils::TryAutoConnectNodes(Graph, SourceNode, TargetNode, bConnectExec, bConnectData);

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetBoolField(TEXT("connected"), bConnected);
	ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
	ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleListGraphs(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	TArray<UEdGraph*> AllGraphs = FCommonUtils::GetAllGraphs(Blueprint);

	TArray<TSharedPtr<FJsonValue>> GraphsArray;
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph) continue;
		GraphsArray.Add(MakeShared<FJsonValueObject>(FCommonUtils::GraphToJson(Graph)));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
	ResultObj->SetArrayField(TEXT("graphs"), GraphsArray);
	ResultObj->SetNumberField(TEXT("count"), GraphsArray.Num());
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleCreateChildBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	FString ChildName;
	if (!Params->TryGetStringField(TEXT("name"), ChildName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString ParentBlueprintPath;
	if (!Params->TryGetStringField(TEXT("parent_blueprint"), ParentBlueprintPath))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'parent_blueprint' parameter"));
	}

	FString AssetPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("asset_path"), AssetPath);

	if (!AssetPath.EndsWith(TEXT("/")))
	{
		AssetPath += TEXT("/");
	}

	// Load parent blueprint
	UBlueprint* ParentBlueprint = LoadObject<UBlueprint>(nullptr, *ParentBlueprintPath);
	if (!ParentBlueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent blueprint not found: %s"), *ParentBlueprintPath));
	}

	UClass* ParentClass = ParentBlueprint->GeneratedClass;
	if (!ParentClass)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Parent blueprint has no generated class. Compile it first."));
	}

	// Check if asset already exists
	FString FullAssetPath = AssetPath + ChildName;
	if (UEditorAssetLibrary::DoesAssetExist(FullAssetPath))
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint already exists: %s"), *FullAssetPath));
	}

	// Create child blueprint
	UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
	Factory->ParentClass = ParentClass;

	UPackage* Package = CreatePackage(*FullAssetPath);
	UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(
		UBlueprint::StaticClass(),
		Package,
		*ChildName,
		RF_Standalone | RF_Public,
		nullptr,
		GWarn
	));

	if (!NewBlueprint)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to create child blueprint"));
	}

	// Compile and save
	FKismetEditorUtilities::CompileBlueprint(NewBlueprint);
	FAssetRegistryModule::AssetCreated(NewBlueprint);
	Package->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(FullAssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	bool bSaved = UPackage::SavePackage(Package, NewBlueprint, *PackageFileName, SaveArgs);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("name"), ChildName);
	ResultObj->SetStringField(TEXT("path"), FullAssetPath);
	ResultObj->SetStringField(TEXT("parent_blueprint"), ParentBlueprintPath);
	ResultObj->SetStringField(TEXT("parent_class"), ParentClass->GetName());
	ResultObj->SetBoolField(TEXT("saved"), bSaved);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleDeleteBlueprintNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	UEdGraphNode* TargetNode = FCommonUtils::FindNodeByGuidInBlueprint(Blueprint, NodeId);
	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	UEdGraph* OwningGraph = TargetNode->GetGraph();
	FString NodeTitle = TargetNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
	OwningGraph->RemoveNode(TargetNode);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("removed_node_id"), NodeId);
	ResultObj->SetStringField(TEXT("removed_node_title"), NodeTitle);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleDeleteBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	FName VarName(*VariableName);
	int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName);
	if (VarIndex == INDEX_NONE)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Variable not found: %s"), *VariableName));
	}

	FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, VarName);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("removed_variable"), VariableName);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleDeleteComponentFromBlueprint(const TSharedPtr<FJsonObject>& Params)
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

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	if (!Blueprint->SimpleConstructionScript)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Blueprint has no SimpleConstructionScript"));
	}

	USCS_Node* TargetNode = nullptr;
	TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
	for (USCS_Node* Node : AllNodes)
	{
		if (Node && Node->GetVariableName() == FName(*ComponentName))
		{
			TargetNode = Node;
			break;
		}
	}

	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
	}

	Blueprint->SimpleConstructionScript->RemoveNode(TargetNode);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("removed_component"), ComponentName);
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleDisconnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
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

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	UEdGraphNode* TargetNode = FCommonUtils::FindNodeByGuidInBlueprint(Blueprint, NodeId);
	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	UEdGraphPin* TargetPin = nullptr;
	for (UEdGraphPin* Pin : TargetNode->Pins)
	{
		if (Pin && Pin->PinName.ToString() == PinName)
		{
			TargetPin = Pin;
			break;
		}
	}

	if (!TargetPin)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Pin not found: %s"), *PinName));
	}

	int32 DisconnectedCount = TargetPin->LinkedTo.Num();
	TargetPin->BreakAllPinLinks();
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("node_id"), NodeId);
	ResultObj->SetStringField(TEXT("pin_name"), PinName);
	ResultObj->SetNumberField(TEXT("disconnected_links"), DisconnectedCount);
	return ResultObj;
}

// ============================================================================
// Declarative Graph Builder - Atomic node creation with semantic naming
// ============================================================================

TSharedPtr<FJsonObject> FBlueprintCommands::HandleBuildAbilityGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	const TArray<TSharedPtr<FJsonValue>>* NodesArray;
	if (!Params->TryGetArrayField(TEXT("nodes"), NodesArray))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'nodes' array"));
	}

	const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray;
	if (!Params->TryGetArrayField(TEXT("connections"), ConnectionsArray))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'connections' array"));
	}

	FString BlueprintPath = TEXT("/Game/GAS/Abilities/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Blueprint not found: %s in %s"), *BlueprintName, *BlueprintPath));
	}

	UEdGraph* TargetGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		TargetGraph = FCommonUtils::FindGraphByName(Blueprint, GraphName);
	}
	if (!TargetGraph)
	{
		TargetGraph = FCommonUtils::FindOrCreateEventGraph(Blueprint);
	}
	if (!TargetGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Failed to get target graph"));
	}

	FScopedTransaction Transaction(FText::FromString(TEXT("Build Ability Graph")));

	TMap<FString, UEdGraphNode*> NodeRegistry;

	for (UEdGraphNode* ExistingNode : TargetGraph->Nodes)
	{
		if (!ExistingNode) continue;

		if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(ExistingNode))
		{
			NodeRegistry.Add(TEXT("Entry"), ExistingNode);
			NodeRegistry.Add(TEXT("ActivateAbility"), ExistingNode);
		}
		else if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(ExistingNode))
		{
			FString EventName = EventNode->EventReference.GetMemberName().ToString();
			NodeRegistry.Add(EventName, ExistingNode);
		}
	}
	TArray<FString> ValidationErrors;
	for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
	{
		const TSharedPtr<FJsonObject>* NodeObj;
		if (!NodeValue->TryGetObject(NodeObj)) continue;

		FString NodeName;
		if (!(*NodeObj)->TryGetStringField(TEXT("name"), NodeName))
		{
			ValidationErrors.Add(TEXT("Node missing 'name' field"));
			continue;
		}

		FString NodeType;
		if (!(*NodeObj)->TryGetStringField(TEXT("type"), NodeType))
		{
			ValidationErrors.Add(FString::Printf(TEXT("Node '%s' missing 'type' field"), *NodeName));
		}
	}

	if (ValidationErrors.Num() > 0)
	{
		Transaction.Cancel();
		TSharedPtr<FJsonObject> ErrorResult = MakeShared<FJsonObject>();
		ErrorResult->SetBoolField(TEXT("success"), false);
		ErrorResult->SetStringField(TEXT("error"), TEXT("Validation failed"));
		TArray<TSharedPtr<FJsonValue>> ErrorsJson;
		for (const FString& Err : ValidationErrors)
		{
			ErrorsJson.Add(MakeShared<FJsonValueString>(Err));
		}
		ErrorResult->SetArrayField(TEXT("validation_errors"), ErrorsJson);
		return ErrorResult;
	}

	bool bAutoLayout = true;
	Params->TryGetBoolField(TEXT("auto_layout"), bAutoLayout);

	float AutoLayoutBaseX = 0.0f;
	float AutoLayoutBaseY = 0.0f;
	if (bAutoLayout && NodeRegistry.Contains(TEXT("Entry")))
	{
		UEdGraphNode* EntryNode = NodeRegistry[TEXT("Entry")];
		AutoLayoutBaseX = EntryNode->NodePosX + 300.0f;
		AutoLayoutBaseY = EntryNode->NodePosY;
	}
	int32 AutoLayoutNodeIndex = 0;

	// Helper lambda for safe pin allocation with Schema check
	auto SafeAllocatePins = [&TargetGraph, &ValidationErrors](UEdGraphNode* Node, const FString& NodeName) -> bool
	{
		if (!Node || !TargetGraph)
		{
			ValidationErrors.Add(FString::Printf(TEXT("Node '%s': Invalid node or graph"), *NodeName));
			return false;
		}

		const UEdGraphSchema* Schema = TargetGraph->GetSchema();
		if (!Schema)
		{
			ValidationErrors.Add(FString::Printf(TEXT("Node '%s': Graph has no valid schema - cannot allocate pins"), *NodeName));
			TargetGraph->RemoveNode(Node);
			return false;
		}

		Node->AllocateDefaultPins();
		return true;
	};

	TArray<TSharedPtr<FJsonValue>> CreatedNodesJson;
	for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
	{
		const TSharedPtr<FJsonObject>* NodeObj;
		if (!NodeValue->TryGetObject(NodeObj)) continue;

		FString NodeName;
		(*NodeObj)->TryGetStringField(TEXT("name"), NodeName);

		FString NodeType;
		(*NodeObj)->TryGetStringField(TEXT("type"), NodeType);

		FVector2D Position(0.0f, 0.0f);
		if ((*NodeObj)->HasField(TEXT("position")))
		{
			Position = FCommonUtils::GetVector2DFromJson(*NodeObj, TEXT("position"));
		}
		else if (bAutoLayout)
		{
			int32 Col = AutoLayoutNodeIndex % 4;
			int32 Row = AutoLayoutNodeIndex / 4;
			Position.X = AutoLayoutBaseX + Col * 250.0f;
			Position.Y = AutoLayoutBaseY + Row * 150.0f;
			AutoLayoutNodeIndex++;
		}

		UEdGraphNode* CreatedNode = nullptr;

		if (NodeType == TEXT("Entry") || NodeType == TEXT("FunctionEntry"))
		{
			if (NodeRegistry.Contains(NodeName))
			{
				CreatedNode = NodeRegistry[NodeName];
			}
			else if (NodeRegistry.Contains(TEXT("Entry")))
			{
				CreatedNode = NodeRegistry[TEXT("Entry")];
				NodeRegistry.Add(NodeName, CreatedNode);
			}
			else
			{
				for (UEdGraphNode* ExistingNode : TargetGraph->Nodes)
				{
					if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(ExistingNode))
					{
						CreatedNode = ExistingNode;
						NodeRegistry.Add(NodeName, CreatedNode);
						break;
					}
				}
			}
			if (CreatedNode)
			{
				continue;
			}
		}
		else if (NodeType == TEXT("CallFunction") || NodeType == TEXT("function"))
		{
			FString FunctionName;
			(*NodeObj)->TryGetStringField(TEXT("function_name"), FunctionName);

			FString TargetClass;
			(*NodeObj)->TryGetStringField(TEXT("target_class"), TargetClass);

			UClass* FuncClass = nullptr;
			if (!TargetClass.IsEmpty())
			{
				FuncClass = FCommonUtils::FindClassByName(TargetClass);
			}
			if (!FuncClass)
			{
				FuncClass = Blueprint->GeneratedClass;
			}
			if (!FuncClass)
			{
				FuncClass = UGameplayAbility::StaticClass();
			}

			UFunction* Func = FuncClass->FindFunctionByName(*FunctionName);
			if (!Func && !FunctionName.StartsWith(TEXT("K2_")))
			{
				Func = FuncClass->FindFunctionByName(*(TEXT("K2_") + FunctionName));
			}

			if (Func)
			{
				UK2Node_CallFunction* FuncNode = NewObject<UK2Node_CallFunction>(TargetGraph);
				FuncNode->CreateNewGuid();
				FuncNode->SetFromFunction(Func);
				FuncNode->NodePosX = Position.X;
				FuncNode->NodePosY = Position.Y;
				TargetGraph->AddNode(FuncNode, true);
				FuncNode->PostPlacedNewNode();

				if (!SafeAllocatePins(FuncNode, NodeName))
				{
					continue;
				}

				CreatedNode = FuncNode;
			}
			else
			{
				ValidationErrors.Add(FString::Printf(TEXT("Function '%s' not found in class '%s'"),
					*FunctionName, *TargetClass));
			}
		}
		else if (NodeType == TEXT("Branch") || NodeType == TEXT("IfThenElse"))
		{
			UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(TargetGraph);
			BranchNode->CreateNewGuid();
			BranchNode->NodePosX = Position.X;
			BranchNode->NodePosY = Position.Y;
			TargetGraph->AddNode(BranchNode, true);
			BranchNode->PostPlacedNewNode();

			if (!SafeAllocatePins(BranchNode, NodeName))
			{
				continue;
			}

			CreatedNode = BranchNode;
		}
		else if (NodeType == TEXT("Sequence"))
		{
			UK2Node_ExecutionSequence* SeqNode = NewObject<UK2Node_ExecutionSequence>(TargetGraph);
			SeqNode->CreateNewGuid();
			SeqNode->NodePosX = Position.X;
			SeqNode->NodePosY = Position.Y;
			TargetGraph->AddNode(SeqNode, true);
			SeqNode->PostPlacedNewNode();

			if (!SafeAllocatePins(SeqNode, NodeName))
			{
				continue;
			}

			CreatedNode = SeqNode;
		}
		else if (NodeType == TEXT("Self"))
		{
			UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(TargetGraph);
			SelfNode->CreateNewGuid();
			SelfNode->NodePosX = Position.X;
			SelfNode->NodePosY = Position.Y;
			TargetGraph->AddNode(SelfNode, true);
			SelfNode->PostPlacedNewNode();

			if (!SafeAllocatePins(SelfNode, NodeName))
			{
				continue;
			}

			CreatedNode = SelfNode;
		}
		else if (NodeType == TEXT("VariableGet") || NodeType == TEXT("GetVariable"))
		{
			FString VariableName;
			(*NodeObj)->TryGetStringField(TEXT("variable_name"), VariableName);

			UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(TargetGraph);
			GetNode->CreateNewGuid();
			GetNode->VariableReference.SetSelfMember(FName(*VariableName));
			GetNode->NodePosX = Position.X;
			GetNode->NodePosY = Position.Y;
			TargetGraph->AddNode(GetNode, true);
			GetNode->PostPlacedNewNode();

			if (!SafeAllocatePins(GetNode, NodeName))
			{
				continue;
			}

			GetNode->ReconstructNode();
			CreatedNode = GetNode;
		}
		else if (NodeType == TEXT("VariableSet") || NodeType == TEXT("SetVariable"))
		{
			FString VariableName;
			(*NodeObj)->TryGetStringField(TEXT("variable_name"), VariableName);

			UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(TargetGraph);
			SetNode->CreateNewGuid();
			SetNode->VariableReference.SetSelfMember(FName(*VariableName));
			SetNode->NodePosX = Position.X;
			SetNode->NodePosY = Position.Y;
			TargetGraph->AddNode(SetNode, true);
			SetNode->PostPlacedNewNode();

			if (!SafeAllocatePins(SetNode, NodeName))
			{
				continue;
			}

			SetNode->ReconstructNode();
			CreatedNode = SetNode;
		}
		else if (NodeType == TEXT("SpawnActor") || NodeType == TEXT("SpawnActorFromClass"))
		{
			FString ActorClassPath;
			(*NodeObj)->TryGetStringField(TEXT("actor_class"), ActorClassPath);

			// Use CallFunction to GameplayStatics::BeginDeferredActorSpawnFromClass instead of UK2Node_SpawnActorFromClass
			// UK2Node_SpawnActorFromClass crashes in Function Graph because it requires Blueprint context for World Context Pin
			UFunction* SpawnFunc = UGameplayStatics::StaticClass()->FindFunctionByName(TEXT("BeginDeferredActorSpawnFromClass"));
			if (SpawnFunc)
			{
				UK2Node_CallFunction* SpawnNode = NewObject<UK2Node_CallFunction>(TargetGraph);
				SpawnNode->CreateNewGuid();
				SpawnNode->SetFromFunction(SpawnFunc);
				SpawnNode->NodePosX = Position.X;
				SpawnNode->NodePosY = Position.Y;
				TargetGraph->AddNode(SpawnNode, true);
				SpawnNode->PostPlacedNewNode();

				if (!SafeAllocatePins(SpawnNode, NodeName))
				{
					continue;
				}

				if (!ActorClassPath.IsEmpty())
				{
					UClass* ActorClass = LoadClass<AActor>(nullptr, *ActorClassPath);
					if (ActorClass)
					{
						// Set the ActorClass pin
						UEdGraphPin* ClassPin = SpawnNode->FindPin(TEXT("ActorClass"));
						if (ClassPin)
						{
							ClassPin->DefaultObject = ActorClass;
						}
					}
				}

				CreatedNode = SpawnNode;
			}
			else
			{
				ValidationErrors.Add(FString::Printf(TEXT("Node '%s': Failed to find BeginDeferredActorSpawnFromClass function"), *NodeName));
				continue;
			}
		}
		else if (NodeType == TEXT("AbilityTask") || NodeType == TEXT("LatentTask"))
		{
			FString TaskClass;
			(*NodeObj)->TryGetStringField(TEXT("task_class"), TaskClass);

			FString FunctionName;
			(*NodeObj)->TryGetStringField(TEXT("function_name"), FunctionName);

			UClass* TaskClassPtr = FCommonUtils::FindClassByName(TaskClass);
			if (TaskClassPtr)
			{
				UFunction* ProxyFunc = TaskClassPtr->FindFunctionByName(*FunctionName);
				if (ProxyFunc)
				{
					UK2Node_CallFunction* TaskNode = NewObject<UK2Node_CallFunction>(TargetGraph);
					TaskNode->CreateNewGuid();
					TaskNode->SetFromFunction(ProxyFunc);
					TaskNode->NodePosX = Position.X;
					TaskNode->NodePosY = Position.Y;
					TargetGraph->AddNode(TaskNode, true);
					TaskNode->PostPlacedNewNode();

					if (!SafeAllocatePins(TaskNode, NodeName))
					{
						continue;
					}

					CreatedNode = TaskNode;
				}
			}
		}
		else if (NodeType == TEXT("Cast") || NodeType == TEXT("DynamicCast"))
		{
			FString TargetTypePath;
			if (!(*NodeObj)->TryGetStringField(TEXT("target_class"), TargetTypePath))
			{
				(*NodeObj)->TryGetStringField(TEXT("target_type"), TargetTypePath);
			}

			// TargetType must be set before node creation for safe pin allocation
			UClass* TargetType = nullptr;
			if (!TargetTypePath.IsEmpty())
			{
				TargetType = FCommonUtils::FindClassByName(TargetTypePath);
				if (!TargetType)
				{
					TargetType = LoadClass<UObject>(nullptr, *TargetTypePath);
				}
			}

			if (!TargetType)
			{
				ValidationErrors.Add(FString::Printf(TEXT("Cast node '%s': Failed to resolve target type '%s'"), *NodeName, *TargetTypePath));
				continue;
			}

			UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(TargetGraph);
			CastNode->CreateNewGuid();
			CastNode->TargetType = TargetType;  // Set BEFORE AddNode
			CastNode->NodePosX = Position.X;
			CastNode->NodePosY = Position.Y;
			TargetGraph->AddNode(CastNode, true);
			CastNode->PostPlacedNewNode();

			if (!SafeAllocatePins(CastNode, NodeName))
			{
				continue;
			}

			CreatedNode = CastNode;
		}
		else if (NodeType == TEXT("MakeStruct"))
		{
			FString StructPath;
			(*NodeObj)->TryGetStringField(TEXT("struct_type"), StructPath);

			if (StructPath.IsEmpty())
			{
				ValidationErrors.Add(FString::Printf(TEXT("MakeStruct node '%s' missing 'struct_type'"), *NodeName));
				continue;
			}

			UScriptStruct* Struct = nullptr;
			if (StructPath.Contains(TEXT("/")))
			{
				Struct = LoadObject<UScriptStruct>(nullptr, *StructPath);
			}
			else
			{
				static const TMap<FString, FString> CommonStructPaths = {
					{TEXT("Transform"), TEXT("/Script/CoreUObject.Transform")},
					{TEXT("Vector"), TEXT("/Script/CoreUObject.Vector")},
					{TEXT("Rotator"), TEXT("/Script/CoreUObject.Rotator")},
					{TEXT("LinearColor"), TEXT("/Script/CoreUObject.LinearColor")},
					{TEXT("Color"), TEXT("/Script/CoreUObject.Color")},
					{TEXT("Vector2D"), TEXT("/Script/CoreUObject.Vector2D")},
					{TEXT("HitResult"), TEXT("/Script/Engine.HitResult")},
					{TEXT("GameplayTag"), TEXT("/Script/GameplayTags.GameplayTag")},
					{TEXT("GameplayTagContainer"), TEXT("/Script/GameplayTags.GameplayTagContainer")},
					{TEXT("GameplayEffectSpec"), TEXT("/Script/GameplayAbilities.GameplayEffectSpec")},
					{TEXT("GameplayAbilitySpec"), TEXT("/Script/GameplayAbilities.GameplayAbilitySpec")},
					{TEXT("GameplayEventData"), TEXT("/Script/GameplayAbilities.GameplayEventData")}
				};

				if (const FString* FullPath = CommonStructPaths.Find(StructPath))
				{
					Struct = LoadObject<UScriptStruct>(nullptr, **FullPath);
				}
				else
				{
					Struct = FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/CoreUObject.%s"), *StructPath));
					if (!Struct)
					{
						Struct = FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *StructPath));
					}
				}
			}

			if (!Struct)
			{
				ValidationErrors.Add(FString::Printf(TEXT("MakeStruct node '%s': struct_type '%s' not found"), *NodeName, *StructPath));
				continue;
			}

			UK2Node_MakeStruct* MakeNode = NewObject<UK2Node_MakeStruct>(TargetGraph);
			MakeNode->CreateNewGuid();
			MakeNode->StructType = Struct;
			MakeNode->NodePosX = Position.X;
			MakeNode->NodePosY = Position.Y;
			TargetGraph->AddNode(MakeNode, true);
			MakeNode->PostPlacedNewNode();

			if (!SafeAllocatePins(MakeNode, NodeName))
			{
				continue;
			}

			CreatedNode = MakeNode;
		}
		else if (NodeType == TEXT("BreakStruct"))
		{
			FString StructPath;
			(*NodeObj)->TryGetStringField(TEXT("struct_type"), StructPath);

			if (StructPath.IsEmpty())
			{
				ValidationErrors.Add(FString::Printf(TEXT("BreakStruct node '%s' missing 'struct_type'"), *NodeName));
				continue;
			}

			UScriptStruct* Struct = nullptr;
			if (StructPath.Contains(TEXT("/")))
			{
				Struct = LoadObject<UScriptStruct>(nullptr, *StructPath);
			}
			else
			{
				static const TMap<FString, FString> CommonStructPaths = {
					{TEXT("Transform"), TEXT("/Script/CoreUObject.Transform")},
					{TEXT("Vector"), TEXT("/Script/CoreUObject.Vector")},
					{TEXT("Rotator"), TEXT("/Script/CoreUObject.Rotator")},
					{TEXT("LinearColor"), TEXT("/Script/CoreUObject.LinearColor")},
					{TEXT("Color"), TEXT("/Script/CoreUObject.Color")},
					{TEXT("Vector2D"), TEXT("/Script/CoreUObject.Vector2D")},
					{TEXT("HitResult"), TEXT("/Script/Engine.HitResult")},
					{TEXT("GameplayTag"), TEXT("/Script/GameplayTags.GameplayTag")},
					{TEXT("GameplayTagContainer"), TEXT("/Script/GameplayTags.GameplayTagContainer")},
					{TEXT("GameplayEffectSpec"), TEXT("/Script/GameplayAbilities.GameplayEffectSpec")},
					{TEXT("GameplayAbilitySpec"), TEXT("/Script/GameplayAbilities.GameplayAbilitySpec")},
					{TEXT("GameplayEventData"), TEXT("/Script/GameplayAbilities.GameplayEventData")}
				};

				if (const FString* FullPath = CommonStructPaths.Find(StructPath))
				{
					Struct = LoadObject<UScriptStruct>(nullptr, **FullPath);
				}
				else
				{
					Struct = FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/CoreUObject.%s"), *StructPath));
					if (!Struct)
					{
						Struct = FindObject<UScriptStruct>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *StructPath));
					}
				}
			}

			if (!Struct)
			{
				ValidationErrors.Add(FString::Printf(TEXT("BreakStruct node '%s': struct_type '%s' not found"), *NodeName, *StructPath));
				continue;
			}

			UK2Node_BreakStruct* BreakNode = NewObject<UK2Node_BreakStruct>(TargetGraph);
			BreakNode->CreateNewGuid();
			BreakNode->StructType = Struct;
			BreakNode->NodePosX = Position.X;
			BreakNode->NodePosY = Position.Y;
			TargetGraph->AddNode(BreakNode, true);
			BreakNode->PostPlacedNewNode();

			if (!SafeAllocatePins(BreakNode, NodeName))
			{
				continue;
			}

			CreatedNode = BreakNode;
		}
		else if (NodeType == TEXT("CommitAbility"))
		{
			UClass* GAClass = UGameplayAbility::StaticClass();
			UFunction* Func = GAClass->FindFunctionByName(TEXT("K2_CommitAbility"));
			if (Func)
			{
				UK2Node_CallFunction* FuncNode = NewObject<UK2Node_CallFunction>(TargetGraph);
				FuncNode->CreateNewGuid();
				FuncNode->SetFromFunction(Func);
				FuncNode->NodePosX = Position.X;
				FuncNode->NodePosY = Position.Y;
				TargetGraph->AddNode(FuncNode, true);
				FuncNode->PostPlacedNewNode();

				if (!SafeAllocatePins(FuncNode, NodeName))
				{
					continue;
				}

				CreatedNode = FuncNode;
			}
		}
		else if (NodeType == TEXT("EndAbility"))
		{
			UClass* GAClass = UGameplayAbility::StaticClass();
			UFunction* Func = GAClass->FindFunctionByName(TEXT("K2_EndAbility"));
			if (Func)
			{
				UK2Node_CallFunction* FuncNode = NewObject<UK2Node_CallFunction>(TargetGraph);
				FuncNode->CreateNewGuid();
				FuncNode->SetFromFunction(Func);
				FuncNode->NodePosX = Position.X;
				FuncNode->NodePosY = Position.Y;
				TargetGraph->AddNode(FuncNode, true);
				FuncNode->PostPlacedNewNode();

				if (!SafeAllocatePins(FuncNode, NodeName))
				{
					continue;
				}

				CreatedNode = FuncNode;
			}
		}
		else if (NodeType == TEXT("CheckCooldown"))
		{
			UClass* GAClass = UGameplayAbility::StaticClass();
			UFunction* Func = GAClass->FindFunctionByName(TEXT("K2_CheckAbilityCooldown"));
			if (Func)
			{
				UK2Node_CallFunction* FuncNode = NewObject<UK2Node_CallFunction>(TargetGraph);
				FuncNode->CreateNewGuid();
				FuncNode->SetFromFunction(Func);
				FuncNode->NodePosX = Position.X;
				FuncNode->NodePosY = Position.Y;
				TargetGraph->AddNode(FuncNode, true);
				FuncNode->PostPlacedNewNode();

				if (!SafeAllocatePins(FuncNode, NodeName))
				{
					continue;
				}

				CreatedNode = FuncNode;
			}
		}
		else if (NodeType == TEXT("CheckCost"))
		{
			UClass* GAClass = UGameplayAbility::StaticClass();
			UFunction* Func = GAClass->FindFunctionByName(TEXT("K2_CheckAbilityCost"));
			if (Func)
			{
				UK2Node_CallFunction* FuncNode = NewObject<UK2Node_CallFunction>(TargetGraph);
				FuncNode->CreateNewGuid();
				FuncNode->SetFromFunction(Func);
				FuncNode->NodePosX = Position.X;
				FuncNode->NodePosY = Position.Y;
				TargetGraph->AddNode(FuncNode, true);
				FuncNode->PostPlacedNewNode();

				if (!SafeAllocatePins(FuncNode, NodeName))
				{
					continue;
				}

				CreatedNode = FuncNode;
			}
		}
		else if (NodeType == TEXT("ApplyEffect") || NodeType == TEXT("ApplyEffectToOwner") || NodeType == TEXT("ApplyGameplayEffectToOwner"))
		{
			UClass* GAClass = UGameplayAbility::StaticClass();
			UFunction* Func = GAClass->FindFunctionByName(TEXT("BP_ApplyGameplayEffectToOwner"));
			if (!Func)
			{
				Func = GAClass->FindFunctionByName(TEXT("K2_ApplyGameplayEffectToOwner"));
			}
			if (Func)
			{
				UK2Node_CallFunction* FuncNode = NewObject<UK2Node_CallFunction>(TargetGraph);
				FuncNode->CreateNewGuid();
				FuncNode->SetFromFunction(Func);
				FuncNode->NodePosX = Position.X;
				FuncNode->NodePosY = Position.Y;
				TargetGraph->AddNode(FuncNode, true);
				FuncNode->PostPlacedNewNode();

				if (!SafeAllocatePins(FuncNode, NodeName))
				{
					continue;
				}

				CreatedNode = FuncNode;
			}
		}
		else if (NodeType == TEXT("ApplyEffectToTarget") || NodeType == TEXT("ApplyGameplayEffectToTarget"))
		{
			UClass* GAClass = UGameplayAbility::StaticClass();
			UFunction* Func = GAClass->FindFunctionByName(TEXT("BP_ApplyGameplayEffectToTarget"));
			if (!Func)
			{
				Func = GAClass->FindFunctionByName(TEXT("K2_ApplyGameplayEffectToTarget"));
			}
			if (Func)
			{
				UK2Node_CallFunction* FuncNode = NewObject<UK2Node_CallFunction>(TargetGraph);
				FuncNode->CreateNewGuid();
				FuncNode->SetFromFunction(Func);
				FuncNode->NodePosX = Position.X;
				FuncNode->NodePosY = Position.Y;
				TargetGraph->AddNode(FuncNode, true);
				FuncNode->PostPlacedNewNode();

				if (!SafeAllocatePins(FuncNode, NodeName))
				{
					continue;
				}

				CreatedNode = FuncNode;
			}
		}
		else if (NodeType == TEXT("PlayMontage") || NodeType == TEXT("PlayMontageAndWait"))
		{
			UClass* TaskClass = FindObject<UClass>(nullptr, TEXT("/Script/GameplayAbilities.AbilityTask_PlayMontageAndWait"));
			if (!TaskClass)
			{
				TaskClass = FCommonUtils::FindClassByName(TEXT("UAbilityTask_PlayMontageAndWait"));
			}
			if (TaskClass)
			{
				UFunction* ProxyFunc = TaskClass->FindFunctionByName(TEXT("CreatePlayMontageAndWaitProxy"));
				if (ProxyFunc)
				{
					UK2Node_CallFunction* TaskNode = NewObject<UK2Node_CallFunction>(TargetGraph);
					TaskNode->CreateNewGuid();
					TaskNode->SetFromFunction(ProxyFunc);
					TaskNode->NodePosX = Position.X;
					TaskNode->NodePosY = Position.Y;
					TargetGraph->AddNode(TaskNode, true);
					TaskNode->PostPlacedNewNode();

					if (!SafeAllocatePins(TaskNode, NodeName))
					{
						continue;
					}

					CreatedNode = TaskNode;
				}
			}
		}
		else if (NodeType == TEXT("WaitGameplayEvent"))
		{
			UClass* TaskClass = FindObject<UClass>(nullptr, TEXT("/Script/GameplayAbilities.AbilityTask_WaitGameplayEvent"));
			if (!TaskClass)
			{
				TaskClass = FCommonUtils::FindClassByName(TEXT("UAbilityTask_WaitGameplayEvent"));
			}
			if (TaskClass)
			{
				UFunction* ProxyFunc = TaskClass->FindFunctionByName(TEXT("WaitGameplayEvent"));
				if (ProxyFunc)
				{
					UK2Node_CallFunction* TaskNode = NewObject<UK2Node_CallFunction>(TargetGraph);
					TaskNode->CreateNewGuid();
					TaskNode->SetFromFunction(ProxyFunc);
					TaskNode->NodePosX = Position.X;
					TaskNode->NodePosY = Position.Y;
					TargetGraph->AddNode(TaskNode, true);
					TaskNode->PostPlacedNewNode();

					if (!SafeAllocatePins(TaskNode, NodeName))
					{
						continue;
					}
					CreatedNode = TaskNode;
				}
			}
		}
		else if (NodeType == TEXT("ForEachLoop") || NodeType == TEXT("ForEachLoopWithBreak"))
		{
			// ForEachLoop is a macro - find it in the engine macro library
			FString MacroPath = NodeType == TEXT("ForEachLoopWithBreak")
				? TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:ForEachLoopWithBreak")
				: TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:ForEachLoop");

			UEdGraph* MacroGraph = LoadObject<UEdGraph>(nullptr, *MacroPath);
			if (MacroGraph)
			{
				UK2Node_MacroInstance* MacroNode = NewObject<UK2Node_MacroInstance>(TargetGraph);
				MacroNode->CreateNewGuid();
				MacroNode->SetMacroGraph(MacroGraph);
				MacroNode->NodePosX = Position.X;
				MacroNode->NodePosY = Position.Y;
				TargetGraph->AddNode(MacroNode, true);
				MacroNode->PostPlacedNewNode();

				if (!SafeAllocatePins(MacroNode, NodeName))
				{
					continue;
				}

				CreatedNode = MacroNode;
			}
		}
		else
		{
			CreatedNode = FCommonUtils::CreateNodeByClassName(TargetGraph, NodeType, Position);
			if (CreatedNode)
			{
				FCommonUtils::InitializeNodeFromParams(CreatedNode, *NodeObj);
			}
		}

		if (CreatedNode)
		{
			NodeRegistry.Add(NodeName, CreatedNode);

			TSharedPtr<FJsonObject> NodeInfo = MakeShared<FJsonObject>();
			NodeInfo->SetStringField(TEXT("name"), NodeName);
			NodeInfo->SetStringField(TEXT("node_id"), CreatedNode->NodeGuid.ToString());
			NodeInfo->SetStringField(TEXT("node_class"), CreatedNode->GetClass()->GetName());
			CreatedNodesJson.Add(MakeShared<FJsonValueObject>(NodeInfo));
		}
		else
		{
			ValidationErrors.Add(FString::Printf(TEXT("Failed to create node '%s' of type '%s'"),
				*NodeName, *NodeType));
		}
	}

	if (ValidationErrors.Num() > 0)
	{
		Transaction.Cancel();
		TSharedPtr<FJsonObject> ErrorResult = MakeShared<FJsonObject>();
		ErrorResult->SetBoolField(TEXT("success"), false);
		ErrorResult->SetStringField(TEXT("error"), TEXT("Node creation failed"));
		TArray<TSharedPtr<FJsonValue>> ErrorsJson;
		for (const FString& Err : ValidationErrors)
		{
			ErrorsJson.Add(MakeShared<FJsonValueString>(Err));
		}
		ErrorResult->SetArrayField(TEXT("errors"), ErrorsJson);
		return ErrorResult;
	}

	TArray<TSharedPtr<FJsonValue>> CreatedConnectionsJson;
	TArray<FString> ConnectionErrors;

	for (const TSharedPtr<FJsonValue>& ConnValue : *ConnectionsArray)
	{
		const TSharedPtr<FJsonObject>* ConnObj;
		if (!ConnValue->TryGetObject(ConnObj)) continue;

		FString FromSpec, ToSpec;
		if (!(*ConnObj)->TryGetStringField(TEXT("source"), FromSpec))
		{
			(*ConnObj)->TryGetStringField(TEXT("from"), FromSpec);
		}
		if (!(*ConnObj)->TryGetStringField(TEXT("target"), ToSpec))
		{
			(*ConnObj)->TryGetStringField(TEXT("to"), ToSpec);
		}

		FString FromNodeName, FromPinName;
		FString ToNodeName, ToPinName;

		if (!FromSpec.Split(TEXT("."), &FromNodeName, &FromPinName))
		{
			FromNodeName = FromSpec;
			FromPinName = TEXT("then");
		}

		if (!ToSpec.Split(TEXT("."), &ToNodeName, &ToPinName))
		{
			ToNodeName = ToSpec;
			ToPinName = TEXT("execute");
		}

		UEdGraphNode** FromNodePtr = NodeRegistry.Find(FromNodeName);
		UEdGraphNode** ToNodePtr = NodeRegistry.Find(ToNodeName);

		if (!FromNodePtr || !*FromNodePtr)
		{
			ConnectionErrors.Add(FString::Printf(TEXT("Source node '%s' not found"), *FromNodeName));
			continue;
		}

		if (!ToNodePtr || !*ToNodePtr)
		{
			ConnectionErrors.Add(FString::Printf(TEXT("Target node '%s' not found"), *ToNodeName));
			continue;
		}

		UEdGraphNode* FromNode = *FromNodePtr;
		UEdGraphNode* ToNode = *ToNodePtr;

		UEdGraphPin* FromPin = nullptr;
		UEdGraphPin* ToPin = nullptr;

		auto FindPinFlexible = [](UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction) -> UEdGraphPin*
		{
			if (!Node) return nullptr;

			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->Direction == Direction && Pin->PinName.ToString() == PinName)
				{
					return Pin;
				}
			}

			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->Direction == Direction &&
					Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase))
				{
					return Pin;
				}
			}

			FString AltName = PinName;
			if (PinName.Equals(TEXT("execute"), ESearchCase::IgnoreCase))
			{
				AltName = UEdGraphSchema_K2::PN_Execute.ToString();
			}
			else if (PinName.Equals(TEXT("then"), ESearchCase::IgnoreCase))
			{
				AltName = UEdGraphSchema_K2::PN_Then.ToString();
			}
			else if (PinName.Equals(TEXT("ReturnValue"), ESearchCase::IgnoreCase))
			{
				AltName = UEdGraphSchema_K2::PN_ReturnValue.ToString();
			}
			else if (PinName.Equals(TEXT("Condition"), ESearchCase::IgnoreCase))
			{
				// For Branch nodes
				if (UK2Node_IfThenElse* BranchNode = Cast<UK2Node_IfThenElse>(Node))
				{
					return BranchNode->GetConditionPin();
				}
			}
			else if (PinName.Equals(TEXT("True"), ESearchCase::IgnoreCase))
			{
				if (UK2Node_IfThenElse* BranchNode = Cast<UK2Node_IfThenElse>(Node))
				{
					return BranchNode->GetThenPin();
				}
			}
			else if (PinName.Equals(TEXT("False"), ESearchCase::IgnoreCase))
			{
				if (UK2Node_IfThenElse* BranchNode = Cast<UK2Node_IfThenElse>(Node))
				{
					return BranchNode->GetElsePin();
				}
			}

			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->Direction == Direction && Pin->PinName.ToString() == AltName)
				{
					return Pin;
				}
			}

			return nullptr;
		};

		FromPin = FindPinFlexible(FromNode, FromPinName, EGPD_Output);
		ToPin = FindPinFlexible(ToNode, ToPinName, EGPD_Input);

		if (!FromPin)
		{
			TArray<FString> AvailablePins;
			for (UEdGraphPin* Pin : FromNode->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Output)
				{
					AvailablePins.Add(Pin->PinName.ToString());
				}
			}
			ConnectionErrors.Add(FString::Printf(
				TEXT("Pin '%s' not found on node '%s'. Available: [%s]"),
				*FromPinName, *FromNodeName, *FString::Join(AvailablePins, TEXT(", "))));
			continue;
		}

		if (!ToPin)
		{
			TArray<FString> AvailablePins;
			for (UEdGraphPin* Pin : ToNode->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Input)
				{
					AvailablePins.Add(Pin->PinName.ToString());
				}
			}
			ConnectionErrors.Add(FString::Printf(
				TEXT("Pin '%s' not found on node '%s'. Available: [%s]"),
				*ToPinName, *ToNodeName, *FString::Join(AvailablePins, TEXT(", "))));
			continue;
		}

		FromPin->MakeLinkTo(ToPin);

		// Propagate type for Wildcard pins (e.g., MakeArray)
		auto NeedsReconstruct = [](UEdGraphPin* Pin) -> bool
		{
			if (!Pin) return false;
			FName Cat = Pin->PinType.PinCategory;
			return Cat == UEdGraphSchema_K2::PC_Wildcard ||
				   Cat == UEdGraphSchema_K2::PC_Class ||
				   Cat == UEdGraphSchema_K2::PC_Object;
		};

		if (NeedsReconstruct(FromPin))
		{
			FromNode->PinConnectionListChanged(FromPin);
			FromNode->ReconstructNode();
		}
		if (NeedsReconstruct(ToPin))
		{
			ToNode->PinConnectionListChanged(ToPin);
			ToNode->ReconstructNode();
		}

		bool bConnected = FromPin->LinkedTo.Contains(ToPin);

		TSharedPtr<FJsonObject> ConnInfo = MakeShared<FJsonObject>();
		ConnInfo->SetStringField(TEXT("from"), FromSpec);
		ConnInfo->SetStringField(TEXT("to"), ToSpec);
		ConnInfo->SetBoolField(TEXT("success"), bConnected);
		CreatedConnectionsJson.Add(MakeShared<FJsonValueObject>(ConnInfo));
	}

	if (ConnectionErrors.Num() > 0)
	{
		Transaction.Cancel();
		TSharedPtr<FJsonObject> ErrorResult = MakeShared<FJsonObject>();
		ErrorResult->SetBoolField(TEXT("success"), false);
		ErrorResult->SetStringField(TEXT("error"), TEXT("Connection failed"));
		TArray<TSharedPtr<FJsonValue>> ErrorsJson;
		for (const FString& Err : ConnectionErrors)
		{
			ErrorsJson.Add(MakeShared<FJsonValueString>(Err));
		}
		ErrorResult->SetArrayField(TEXT("errors"), ErrorsJson);
		ErrorResult->SetArrayField(TEXT("created_nodes"), CreatedNodesJson);
		return ErrorResult;
	}

	for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
	{
		const TSharedPtr<FJsonObject>* NodeObj;
		if (!NodeValue->TryGetObject(NodeObj)) continue;

		FString NodeName;
		(*NodeObj)->TryGetStringField(TEXT("name"), NodeName);

		const TSharedPtr<FJsonObject>* PinDefaults;
		if ((*NodeObj)->TryGetObjectField(TEXT("pin_defaults"), PinDefaults))
		{
			UEdGraphNode** NodePtr = NodeRegistry.Find(NodeName);
			if (NodePtr && *NodePtr)
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*PinDefaults)->Values)
				{
					FString PinName = Pair.Key;
					FString DefaultValue = Pair.Value->AsString();

					for (UEdGraphPin* Pin : (*NodePtr)->Pins)
					{
						if (Pin && Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase))
						{
							Pin->DefaultValue = DefaultValue;
							break;
						}
					}
				}
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* TopLevelPinDefaults;
	if (Params->TryGetArrayField(TEXT("pin_defaults"), TopLevelPinDefaults))
	{
		for (const TSharedPtr<FJsonValue>& DefaultValue : *TopLevelPinDefaults)
		{
			const TSharedPtr<FJsonObject>* DefaultObj;
			if (!DefaultValue->TryGetObject(DefaultObj)) continue;

			FString NodeName, PinName, Value;
			(*DefaultObj)->TryGetStringField(TEXT("node"), NodeName);
			(*DefaultObj)->TryGetStringField(TEXT("pin"), PinName);
			(*DefaultObj)->TryGetStringField(TEXT("value"), Value);

			if (NodeName.IsEmpty() || PinName.IsEmpty()) continue;

			UEdGraphNode** NodePtr = NodeRegistry.Find(NodeName);
			if (NodePtr && *NodePtr)
			{
				for (UEdGraphPin* Pin : (*NodePtr)->Pins)
				{
					if (Pin && Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase))
					{
						Pin->DefaultValue = Value;
						break;
					}
				}
			}
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
	ResultObj->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
	ResultObj->SetNumberField(TEXT("nodes_created"), CreatedNodesJson.Num());
	ResultObj->SetNumberField(TEXT("connections_created"), CreatedConnectionsJson.Num());
	ResultObj->SetArrayField(TEXT("nodes"), CreatedNodesJson);
	ResultObj->SetArrayField(TEXT("connections"), CreatedConnectionsJson);

	TSharedPtr<FJsonObject> RegistryObj = MakeShared<FJsonObject>();
	for (const TPair<FString, UEdGraphNode*>& Pair : NodeRegistry)
	{
		if (Pair.Value)
		{
			RegistryObj->SetStringField(Pair.Key, Pair.Value->NodeGuid.ToString());
		}
	}
	ResultObj->SetObjectField(TEXT("node_registry"), RegistryObj);

	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddPin(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	// Optional parameters
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString PinName;
	Params->TryGetStringField(TEXT("pin_name"), PinName);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	UEdGraphNode* TargetNode = FCommonUtils::FindNodeByGuidInBlueprint(Blueprint, NodeId);
	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found with id: %s"), *NodeId));
	}

	// Check if node supports adding pins
	UK2Node_ExecutionSequence* SeqNode = Cast<UK2Node_ExecutionSequence>(TargetNode);
	UK2Node_CommutativeAssociativeBinaryOperator* BinaryOpNode = Cast<UK2Node_CommutativeAssociativeBinaryOperator>(TargetNode);
	UK2Node_MakeArray* MakeArrayNode = Cast<UK2Node_MakeArray>(TargetNode);
	UK2Node_MakeStruct* MakeStructNode = Cast<UK2Node_MakeStruct>(TargetNode);

	FString NewPinName;
	if (SeqNode)
	{
		SeqNode->AddInputPin();
		// Get the name of the newly added pin
		for (UEdGraphPin* Pin : SeqNode->Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				NewPinName = Pin->PinName.ToString();
			}
		}
	}
	else if (BinaryOpNode)
	{
		BinaryOpNode->AddInputPin();
		NewPinName = TEXT("AddedPin");
	}
	else if (MakeArrayNode)
	{
		MakeArrayNode->AddInputPin();
		NewPinName = FString::Printf(TEXT("[%d]"), MakeArrayNode->NumInputs - 1);
	}
	else
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Node type '%s' does not support adding pins. Supported: ExecutionSequence, CommutativeAssociativeBinaryOperator, MakeArray"),
			*TargetNode->GetClass()->GetName()));
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	// Build response with all current pins
	TArray<TSharedPtr<FJsonValue>> PinsArray;
	for (UEdGraphPin* Pin : TargetNode->Pins)
	{
		TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
		PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
		PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
		PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
		PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("status"), TEXT("success"));
	ResultObj->SetStringField(TEXT("node_id"), NodeId);
	ResultObj->SetStringField(TEXT("node_type"), TargetNode->GetClass()->GetName());
	ResultObj->SetStringField(TEXT("added_pin"), NewPinName);
	ResultObj->SetArrayField(TEXT("pins"), PinsArray);

	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleDeletePin(const TSharedPtr<FJsonObject>& Params)
{
	// Get required parameters
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
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

	// Optional parameters
	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s in path %s"), *BlueprintName, *BlueprintPath));
	}

	UEdGraphNode* TargetNode = FCommonUtils::FindNodeByGuidInBlueprint(Blueprint, NodeId);
	if (!TargetNode)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found with id: %s"), *NodeId));
	}

	// Find the pin to remove
	UEdGraphPin* PinToRemove = nullptr;
	for (UEdGraphPin* Pin : TargetNode->Pins)
	{
		if (Pin->PinName.ToString() == PinName)
		{
			PinToRemove = Pin;
			break;
		}
	}

	if (!PinToRemove)
	{
		TArray<FString> AvailablePins;
		for (UEdGraphPin* Pin : TargetNode->Pins)
		{
			AvailablePins.Add(Pin->PinName.ToString());
		}
		return FCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Pin '%s' not found. Available pins: %s"),
			*PinName, *FString::Join(AvailablePins, TEXT(", "))));
	}

	// Check if node supports removing pins
	UK2Node_ExecutionSequence* SeqNode = Cast<UK2Node_ExecutionSequence>(TargetNode);
	UK2Node_CommutativeAssociativeBinaryOperator* BinaryOpNode = Cast<UK2Node_CommutativeAssociativeBinaryOperator>(TargetNode);
	UK2Node_MakeArray* MakeArrayNode = Cast<UK2Node_MakeArray>(TargetNode);

	bool bRemoved = false;
	if (SeqNode)
	{
		// ExecutionSequence requires at least 2 output pins
		int32 ExecOutputCount = 0;
		for (UEdGraphPin* Pin : SeqNode->Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				ExecOutputCount++;
			}
		}
		if (ExecOutputCount <= 2)
		{
			return FCommonUtils::CreateErrorResponse(TEXT("ExecutionSequence must have at least 2 output pins"));
		}
		SeqNode->RemoveInputPin(PinToRemove);
		bRemoved = true;
	}
	else if (BinaryOpNode)
	{
		int32 InputCount = 0;
		for (UEdGraphPin* Pin : BinaryOpNode->Pins)
		{
			if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
			{
				InputCount++;
			}
		}
		if (InputCount <= 2)
		{
			return FCommonUtils::CreateErrorResponse(TEXT("BinaryOperator must have at least 2 input pins"));
		}
		BinaryOpNode->RemoveInputPin(PinToRemove);
		bRemoved = true;
	}
	else if (MakeArrayNode)
	{
		if (MakeArrayNode->NumInputs <= 1)
		{
			return FCommonUtils::CreateErrorResponse(TEXT("MakeArray must have at least 1 input pin"));
		}
		MakeArrayNode->RemoveInputPin(PinToRemove);
		bRemoved = true;
	}
	else
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Node type '%s' does not support removing pins. Supported: ExecutionSequence, CommutativeAssociativeBinaryOperator, MakeArray"),
			*TargetNode->GetClass()->GetName()));
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	// Build response with remaining pins
	TArray<TSharedPtr<FJsonValue>> PinsArray;
	for (UEdGraphPin* Pin : TargetNode->Pins)
	{
		TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
		PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
		PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
		PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
		PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("status"), TEXT("success"));
	ResultObj->SetStringField(TEXT("node_id"), NodeId);
	ResultObj->SetStringField(TEXT("node_type"), TargetNode->GetClass()->GetName());
	ResultObj->SetStringField(TEXT("deleted_pin"), PinName);
	ResultObj->SetArrayField(TEXT("pins"), PinsArray);

	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleGetClassProperties(const TSharedPtr<FJsonObject>& Params)
{
	FString ClassName;
	if (!Params->TryGetStringField(TEXT("class_name"), ClassName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'class_name' parameter"));
	}

	bool bIncludeInherited = false;
	Params->TryGetBoolField(TEXT("include_inherited"), bIncludeInherited);

	bool bBlueprintVisibleOnly = true;
	Params->TryGetBoolField(TEXT("blueprint_visible_only"), bBlueprintVisibleOnly);

	UClass* TargetClass = FCommonUtils::FindClassByName(ClassName);
	if (!TargetClass)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Class not found: %s"), *ClassName));
	}

	TArray<TSharedPtr<FJsonValue>> Properties;

	EFieldIteratorFlags::SuperClassFlags SuperFlag = bIncludeInherited
		? EFieldIteratorFlags::IncludeSuper
		: EFieldIteratorFlags::ExcludeSuper;

	for (TFieldIterator<FProperty> PropIt(TargetClass, SuperFlag); PropIt; ++PropIt)
	{
		FProperty* Prop = *PropIt;

		if (bBlueprintVisibleOnly)
		{
			bool bVisible = Prop->HasAnyPropertyFlags(CPF_BlueprintVisible | CPF_Edit);
			if (!bVisible)
			{
				continue;
			}
		}

		TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
		PropObj->SetStringField(TEXT("name"), Prop->GetName());
		PropObj->SetStringField(TEXT("type"), Prop->GetCPPType());

		// Access flags
		TArray<FString> Flags;
		if (Prop->HasAnyPropertyFlags(CPF_BlueprintVisible)) Flags.Add(TEXT("BlueprintVisible"));
		if (Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly)) Flags.Add(TEXT("ReadOnly"));
		if (Prop->HasAnyPropertyFlags(CPF_Edit)) Flags.Add(TEXT("EditAnywhere"));
		if (Prop->HasAnyPropertyFlags(CPF_EditConst)) Flags.Add(TEXT("VisibleOnly"));

		TArray<TSharedPtr<FJsonValue>> FlagArray;
		for (const FString& Flag : Flags)
		{
			FlagArray.Add(MakeShared<FJsonValueString>(Flag));
		}
		PropObj->SetArrayField(TEXT("flags"), FlagArray);

		// Category
		FString Category = Prop->GetMetaData(TEXT("Category"));
		if (!Category.IsEmpty())
		{
			PropObj->SetStringField(TEXT("category"), Category);
		}

		// Tooltip
		FString Tooltip = Prop->GetMetaData(TEXT("Tooltip"));
		if (!Tooltip.IsEmpty())
		{
			PropObj->SetStringField(TEXT("tooltip"), Tooltip);
		}

		Properties.Add(MakeShared<FJsonValueObject>(PropObj));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("class"), TargetClass->GetName());
	ResultObj->SetArrayField(TEXT("properties"), Properties);
	ResultObj->SetNumberField(TEXT("count"), Properties.Num());
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleGetBlueprintVariables(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	TArray<TSharedPtr<FJsonValue>> Variables;

	for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
	{
		TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
		VarObj->SetStringField(TEXT("name"), VarDesc.VarName.ToString());
		VarObj->SetStringField(TEXT("type"), VarDesc.VarType.PinCategory.ToString());

		// Object type name for object references
		if (VarDesc.VarType.PinSubCategoryObject.IsValid())
		{
			VarObj->SetStringField(TEXT("object_type"), VarDesc.VarType.PinSubCategoryObject->GetName());
		}

		// Flags
		TArray<FString> Flags;
		if (VarDesc.PropertyFlags & CPF_BlueprintVisible) Flags.Add(TEXT("BlueprintVisible"));
		if (VarDesc.PropertyFlags & CPF_BlueprintReadOnly) Flags.Add(TEXT("ReadOnly"));
		if (VarDesc.PropertyFlags & CPF_Edit) Flags.Add(TEXT("Editable"));
		if (VarDesc.PropertyFlags & CPF_DisableEditOnInstance) Flags.Add(TEXT("ExposeOnSpawn"));

		TArray<TSharedPtr<FJsonValue>> FlagArray;
		for (const FString& Flag : Flags)
		{
			FlagArray.Add(MakeShared<FJsonValueString>(Flag));
		}
		VarObj->SetArrayField(TEXT("flags"), FlagArray);

		// Category
		VarObj->SetStringField(TEXT("category"), VarDesc.Category.ToString());

		Variables.Add(MakeShared<FJsonValueObject>(VarObj));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("blueprint"), BlueprintName);
	ResultObj->SetArrayField(TEXT("variables"), Variables);
	ResultObj->SetNumberField(TEXT("count"), Variables.Num());
	return ResultObj;
}

TSharedPtr<FJsonObject> FBlueprintCommands::HandleAddPropertyGetSetNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString OwnerClass;
	if (!Params->TryGetStringField(TEXT("owner_class"), OwnerClass))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'owner_class' parameter"));
	}

	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
	}

	FString NodeType;
	if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'node_type' parameter (get/set)"));
	}

	FString BlueprintPath = TEXT("/Game/Blueprints/");
	Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);

	FString GraphName;
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	FVector2D NodePosition(0, 0);
	const TArray<TSharedPtr<FJsonValue>>* PositionArray;
	if (Params->TryGetArrayField(TEXT("node_position"), PositionArray) && PositionArray->Num() >= 2)
	{
		NodePosition.X = (*PositionArray)[0]->AsNumber();
		NodePosition.Y = (*PositionArray)[1]->AsNumber();
	}

	// Find Blueprint
	UBlueprint* Blueprint = FCommonUtils::FindBlueprint(BlueprintName, BlueprintPath);
	if (!Blueprint)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Find owner class
	UClass* TargetClass = FCommonUtils::FindClassByName(OwnerClass);
	if (!TargetClass)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Class not found: %s"), *OwnerClass));
	}

	// Verify property exists on class
	FProperty* TargetProp = TargetClass->FindPropertyByName(FName(*PropertyName));
	if (!TargetProp)
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Property '%s' not found on class '%s'"), *PropertyName, *OwnerClass));
	}

	// Find target graph
	UEdGraph* TargetGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			if (Graph->GetName() == GraphName)
			{
				TargetGraph = Graph;
				break;
			}
		}
		if (!TargetGraph)
		{
			for (UEdGraph* Graph : Blueprint->FunctionGraphs)
			{
				if (Graph->GetName() == GraphName)
				{
					TargetGraph = Graph;
					break;
				}
			}
		}
	}
	else
	{
		TargetGraph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
	}

	if (!TargetGraph)
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Could not find target graph"));
	}

	UK2Node* CreatedNode = nullptr;
	FString NodeTypeLower = NodeType.ToLower();

	if (NodeTypeLower == TEXT("get"))
	{
		UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(TargetGraph);
		GetNode->CreateNewGuid();
		GetNode->VariableReference.SetExternalMember(FName(*PropertyName), TargetClass);
		GetNode->NodePosX = NodePosition.X;
		GetNode->NodePosY = NodePosition.Y;
		TargetGraph->AddNode(GetNode, true);
		GetNode->PostPlacedNewNode();
		GetNode->AllocateDefaultPins();
		CreatedNode = GetNode;
	}
	else if (NodeTypeLower == TEXT("set"))
	{
		UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(TargetGraph);
		SetNode->CreateNewGuid();
		SetNode->VariableReference.SetExternalMember(FName(*PropertyName), TargetClass);
		SetNode->NodePosX = NodePosition.X;
		SetNode->NodePosY = NodePosition.Y;
		TargetGraph->AddNode(SetNode, true);
		SetNode->PostPlacedNewNode();
		SetNode->AllocateDefaultPins();
		CreatedNode = SetNode;
	}
	else
	{
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid node_type: %s (expected 'get' or 'set')"), *NodeType));
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

	// Build response with pin info
	TArray<TSharedPtr<FJsonValue>> PinsArray;
	for (UEdGraphPin* Pin : CreatedNode->Pins)
	{
		TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
		PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
		PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
		PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
		PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("node_id"), CreatedNode->NodeGuid.ToString());
	ResultObj->SetStringField(TEXT("node_type"), NodeTypeLower == TEXT("get") ? TEXT("VariableGet") : TEXT("VariableSet"));
	ResultObj->SetStringField(TEXT("owner_class"), OwnerClass);
	ResultObj->SetStringField(TEXT("property_name"), PropertyName);
	ResultObj->SetArrayField(TEXT("pins"), PinsArray);
	return ResultObj;
}