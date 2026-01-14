#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class AActor;
class ALevelInstance;
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_Event;
class UK2Node_CallFunction;
class UK2Node_InputAction;
class UK2Node_Self;
class UK2Node_FunctionEntry;
class UWorldPartition;

// Pin requirement classification
enum class EPinRequirement : uint8
{
	Required,
	Optional,
	NotConnectable
};

// Instruction context for LLM feedback
struct UNREALENGINEMCP_API FInstructionContext
{
	FString Type;
	FString Message;
	TArray<FString> Suggestions;
	FString ActionHint;

	TSharedPtr<FJsonObject> ToJson() const;
	bool IsValid() const { return !Type.IsEmpty(); }
};

// Message constants
namespace McpMessages
{
	static const TCHAR* NodeCreated = TEXT("Node created successfully.");
	static const TCHAR* PinNotFound = TEXT("Pin '%s' not found on node '%s'.");
	static const TCHAR* PropertyNotFound = TEXT("Property '%s' not found.");
	static const TCHAR* MissingRequiredPins = TEXT("Required pins not connected: %s");
	static const TCHAR* ValidationFailed = TEXT("Validation failed. Fix issues before compiling.");
	static const TCHAR* ConnectionFailed = TEXT("Failed to connect pins.");
}

class UNREALENGINEMCP_API FCommonUtils
{
public:
	// JSON
	static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& Message);
	static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);
	static void GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray);
	static void GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray);
	static FVector2D GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
	static FVector GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
	static FRotator GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);

	// Actor
	static TSharedPtr<FJsonValue> ActorToJson(AActor* Actor);
	static TSharedPtr<FJsonObject> ActorToJsonObject(AActor* Actor, bool bDetailed = false);
	static AActor* FindActorByName(UWorld* World, const FString& ActorName);
	static AActor* FindActorByNameWithAutoLoad(UWorld* World, const FString& ActorName, bool& bOutWasAutoLoaded);
	static AActor* TryLoadActorFromWorldPartition(UWorldPartition* WorldPartition, const FString& ActorName);

	// Blueprint
	static UBlueprint* FindBlueprint(const FString& BlueprintName, const FString& BlueprintPath = TEXT("/Game/Blueprints/"));
	static UBlueprint* FindBlueprintByName(const FString& BlueprintName, const FString& BlueprintPath = TEXT("/Game/Blueprints/"));
	static bool SetObjectProperty(UObject* Object, const FString& PropertyName,
	                              const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage);

	// Graph
	static UEdGraph* FindOrCreateEventGraph(UBlueprint* Blueprint);
	static UK2Node_Event* CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& NodePosition);
	static UK2Node_Event* FindExistingEventNode(UEdGraph* Graph, const FString& EventName);
	static UK2Node_CallFunction* CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& NodePosition);
	static bool ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName,
	                               UEdGraphNode* TargetNode, const FString& TargetPinName);
	static UK2Node_InputAction* CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& NodePosition);
	static UK2Node_Self* CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& NodePosition);
	static UEdGraphPin* FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction);
	static UEdGraph* CreateFunctionOverride(UBlueprint* Blueprint, const FString& FunctionName, UK2Node_FunctionEntry*& OutFunctionEntry);

	// Class/Struct lookup
	static UClass* FindClassByName(const FString& ClassName);
	static UScriptStruct* FindStructByName(const FString& StructName);

	// Node response
	static TArray<TSharedPtr<FJsonValue>> NodePinsToJson(UEdGraphNode* Node);
	static TSharedPtr<FJsonObject> CreateNodeResponse(UEdGraphNode* Node, bool bSuccess = true);
	static TSharedPtr<FJsonObject> CreateNodeResponseWithContext(UEdGraphNode* Node, const FInstructionContext& Context);
	static TSharedPtr<FJsonObject> CreateErrorWithInstruction(const FString& ErrorMessage, const FInstructionContext& Context);

	// Pin analysis
	static EPinRequirement GetPinRequirement(UEdGraphPin* Pin);
	static TArray<FString> FindSimilarNames(const FString& Input, const TArray<FString>& Candidates, int32 MaxResults = 5);
	static TArray<FString> GetUnconnectedRequiredPins(UEdGraphNode* Node);
	static TArray<TSharedPtr<FJsonValue>> ValidateBlueprintGraphs(UBlueprint* Blueprint);

	// Graph router
	static UEdGraph* FindGraphByName(UBlueprint* Blueprint, const FString& GraphName);
	static TArray<UEdGraph*> GetAllGraphs(UBlueprint* Blueprint);
	static TSharedPtr<FJsonObject> GraphToJson(UEdGraph* Graph);

	// Node factory
	static UEdGraphNode* CreateNodeByClassName(UEdGraph* Graph, const FString& NodeClassName, const FVector2D& Position);
	static bool InitializeNodeFromParams(UEdGraphNode* Node, const TSharedPtr<FJsonObject>& Params);

	// Property
	static bool SetNodePropertyByPath(UEdGraphNode* Node, const FString& PropertyPath,
	                                   const TSharedPtr<FJsonValue>& Value, FString& OutError);
	static TSharedPtr<FJsonValue> GetNodePropertyByPath(UEdGraphNode* Node, const FString& PropertyPath);

	// Auto connection
	static bool TryAutoConnectNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, UEdGraphNode* TargetNode,
	                                 bool bConnectExec = true, bool bConnectData = false);
	static UEdGraphPin* FindFirstUnconnectedPin(UEdGraphNode* Node, EEdGraphPinDirection Direction, FName PinCategory = NAME_None);

	// Node search
	static UEdGraphNode* FindNodeByGuid(UEdGraph* Graph, const FString& NodeGuid);
	static UEdGraphNode* FindNodeByGuidInBlueprint(UBlueprint* Blueprint, const FString& NodeGuid);

	// Level Instance
	static void ForEachActorInWorld(UWorld* World, TFunction<bool(AActor*, ALevelInstance*)> Callback, bool bIncludeLevelInstances = true);
	static AActor* FindActorByNameIncludingLevelInstances(UWorld* World, const FString& ActorName, ALevelInstance*& OutOwningLevelInstance);
	static TArray<ALevelInstance*> GetAllLevelInstances(UWorld* World);
	static ULevel* GetLevelInstanceLoadedLevel(ALevelInstance* LevelInstance);
};
