#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles Blueprint-related commands (assets and node graph)
 */
class UNREALENGINEMCP_API FBlueprintCommands
{
public:
	FBlueprintCommands();
	~FBlueprintCommands();

	/**
	 * Handle blueprint command
	 * Routes to specific handler based on command type
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// Blueprint asset commands
	TSharedPtr<FJsonObject> HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetComponentProperty(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetMeshMaterialColor(const TSharedPtr<FJsonObject>& Params);

	// Material commands
	TSharedPtr<FJsonObject> HandleApplyMaterialToBlueprint(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetBlueprintMaterialInfo(const TSharedPtr<FJsonObject>& Params);

	// Blueprint node graph commands (merged from BlueprintNodeCommands)
	TSharedPtr<FJsonObject> HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddComponentGetterNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListBlueprintNodes(const TSharedPtr<FJsonObject>& Params);

	// Organization commands
	TSharedPtr<FJsonObject> HandleAddCommentBox(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAnalyzeBlueprint(const TSharedPtr<FJsonObject>& Params);

	// GAS (Gameplay Ability System) commands
	TSharedPtr<FJsonObject> HandleCreateGameplayEffect(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleCreateGameplayAbility(const TSharedPtr<FJsonObject>& Params);

	// GAS AttributeSet commands
	TSharedPtr<FJsonObject> HandleListAttributeSets(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetAttributeSetInfo(const TSharedPtr<FJsonObject>& Params);

	// Additional node tools
	TSharedPtr<FJsonObject> HandleAddBlueprintFlowControlNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetPinDefaultValue(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddBlueprintVariableNode(const TSharedPtr<FJsonObject>& Params);

	// Reflection tools (function/class discovery)
	TSharedPtr<FJsonObject> HandleSearchFunctions(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetClassFunctions(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetClassProperties(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetBlueprintVariables(const TSharedPtr<FJsonObject>& Params);

	// External property node
	TSharedPtr<FJsonObject> HandleAddPropertyGetSetNode(const TSharedPtr<FJsonObject>& Params);

	// Function override
	TSharedPtr<FJsonObject> HandleAddFunctionOverride(const TSharedPtr<FJsonObject>& Params);

	// AbilityTask (Latent node)
	TSharedPtr<FJsonObject> HandleAddAbilityTaskNode(const TSharedPtr<FJsonObject>& Params);

	// Generic node tools
	TSharedPtr<FJsonObject> HandleAddGenericNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetNodeProperty(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleConnectNodes(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListGraphs(const TSharedPtr<FJsonObject>& Params);

	// Child blueprint
	TSharedPtr<FJsonObject> HandleCreateChildBlueprint(const TSharedPtr<FJsonObject>& Params);

	// Declarative graph builder - atomic node creation with semantic naming
	TSharedPtr<FJsonObject> HandleBuildAbilityGraph(const TSharedPtr<FJsonObject>& Params);

	// Deletion commands
	TSharedPtr<FJsonObject> HandleDeleteBlueprintNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeleteBlueprintVariable(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeleteComponentFromBlueprint(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDisconnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params);

	// Dynamic pin management
	TSharedPtr<FJsonObject> HandleAddPin(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeletePin(const TSharedPtr<FJsonObject>& Params);
};