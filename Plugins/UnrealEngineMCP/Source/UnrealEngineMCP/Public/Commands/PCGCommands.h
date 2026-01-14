#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles PCG (Procedural Content Generation) commands
 */
class UNREALENGINEMCP_API FPCGCommands
{
public:
	FPCGCommands();
	~FPCGCommands();

	/**
	 * Handle PCG command
	 * Routes to specific handler based on command type
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// PCG Graph asset commands
	TSharedPtr<FJsonObject> HandleCreatePCGGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAnalyzePCGGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetPCGGraphToComponent(const TSharedPtr<FJsonObject>& Params);

	// PCG node creation commands
	TSharedPtr<FJsonObject> HandleAddPCGSamplerNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddPCGFilterNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddPCGTransformNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddPCGSpawnerNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddPCGAttributeNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddPCGFlowControlNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddPCGGenericNode(const TSharedPtr<FJsonObject>& Params);

	// PCG node list commands
	TSharedPtr<FJsonObject> HandleListPCGNodes(const TSharedPtr<FJsonObject>& Params);

	// PCG node connection commands
	TSharedPtr<FJsonObject> HandleConnectPCGNodes(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDisconnectPCGNodes(const TSharedPtr<FJsonObject>& Params);

	// PCG node deletion
	TSharedPtr<FJsonObject> HandleDeletePCGNode(const TSharedPtr<FJsonObject>& Params);

	// Helper methods
	class UPCGGraph* FindPCGGraph(const FString& GraphName, const FString& GraphPath);
	class UPCGNode* CreatePCGNode(class UPCGGraph* Graph, const FString& SettingsClassName, const FVector2D& Position);
	TSharedPtr<FJsonObject> CreatePCGNodeResponse(class UPCGNode* Node);
};
