#include "UnrealEngineMCPBridge.h"
#include "UnrealEngineMCPRunnable.h"
#include "Commands/EditorCommands.h"
#include "Commands/BlueprintCommands.h"
#include "Commands/PCGCommands.h"
#include "Commands/PythonExecutor.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformTime.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

#define MCP_SERVER_HOST "127.0.0.1"
#define MCP_SERVER_PORT 55557
#define MCP_MAX_COMMANDS_PER_TICK 10
#define MCP_MAX_QUEUE_SIZE 50

UUnrealEngineMCPBridge::UUnrealEngineMCPBridge()
	: bIsRunning(false)
	, ListenerSocket(nullptr)
	, ServerThread(nullptr)
	, Port(MCP_SERVER_PORT)
	, NextRequestId(1)
	, PendingCommandCount(0)
{
	EditorCommandHandler = MakeShared<FEditorCommands>();
	BlueprintCommandHandler = MakeShared<FBlueprintCommands>();
	PCGCommandHandler = MakeShared<FPCGCommands>();
	PythonExecutorHandler = MakeShared<FPythonExecutor>();
}

UUnrealEngineMCPBridge::~UUnrealEngineMCPBridge()
{
	EditorCommandHandler.Reset();
	BlueprintCommandHandler.Reset();
	PCGCommandHandler.Reset();
	PythonExecutorHandler.Reset();
}

void UUnrealEngineMCPBridge::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogTemp, Display, TEXT("UnrealEngineMCPBridge: Initializing with Command Queue pattern"));

	bIsRunning = false;
	ListenerSocket = nullptr;
	ServerThread = nullptr;
	PendingCommandCount = 0;

	Port = MCP_SERVER_PORT;
	FString PortStr;
	if (FParse::Value(FCommandLine::Get(), TEXT("-McpPort="), PortStr))
	{
		Port = FCString::Atoi(*PortStr);
		UE_LOG(LogTemp, Display, TEXT("UnrealEngineMCPBridge: Port overridden to %d"), Port);
	}

	FIPv4Address::Parse(MCP_SERVER_HOST, ServerAddress);

	StartServer();
}

void UUnrealEngineMCPBridge::Deinitialize()
{
	UE_LOG(LogTemp, Display, TEXT("UnrealEngineMCPBridge: Shutting down"));
	StopServer();
}

void UUnrealEngineMCPBridge::Tick(float DeltaTime)
{
	if (bIsRunning)
	{
		ProcessCommandQueue();
	}
}

void UUnrealEngineMCPBridge::ProcessCommandQueue()
{
	FMcpCommandRequest Request;
	int32 ProcessedCount = 0;

	while (CommandQueue.Dequeue(Request) && ProcessedCount < MCP_MAX_COMMANDS_PER_TICK)
	{
		PendingCommandCount--;

		FString Response = ExecuteCommandInternal(Request.CommandType, Request.Params);

		{
			FScopeLock Lock(&ResponseMapLock);
			ResponseMap.Add(Request.RequestId, FMcpCommandResponse(Request.RequestId, Response, true));
		}

		ProcessedCount++;
	}
}

bool UUnrealEngineMCPBridge::EnqueueCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params, uint32& OutRequestId)
{
	if (PendingCommandCount.Load() >= MCP_MAX_QUEUE_SIZE)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealEngineMCPBridge: Queue full (%d), rejecting: %s"),
			PendingCommandCount.Load(), *CommandType);
		return false;
	}

	OutRequestId = NextRequestId++;
	CommandQueue.Enqueue(FMcpCommandRequest(OutRequestId, CommandType, Params));
	PendingCommandCount++;
	return true;
}

bool UUnrealEngineMCPBridge::TryDequeueResponse(uint32 RequestId, FMcpCommandResponse& OutResponse)
{
	FScopeLock Lock(&ResponseMapLock);

	if (FMcpCommandResponse* Found = ResponseMap.Find(RequestId))
	{
		OutResponse = *Found;
		ResponseMap.Remove(RequestId);
		return true;
	}
	return false;
}

bool UUnrealEngineMCPBridge::WaitForResponse(uint32 RequestId, FMcpCommandResponse& OutResponse, float TimeoutSeconds)
{
	double StartTime = FPlatformTime::Seconds();

	while ((FPlatformTime::Seconds() - StartTime) < TimeoutSeconds)
	{
		if (TryDequeueResponse(RequestId, OutResponse))
		{
			return true;
		}
		FPlatformProcess::Sleep(0.001f);  // 1ms sleep
	}

	return false;
}

FString UUnrealEngineMCPBridge::ExecuteCommandInternal(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	UE_LOG(LogTemp, Display, TEXT("UnrealEngineMCPBridge: Executing command: %s"), *CommandType);

	TSharedPtr<FJsonObject> ResponseJson = MakeShared<FJsonObject>();

	try
	{
		TSharedPtr<FJsonObject> ResultJson;

		if (CommandType == TEXT("ping"))
		{
			ResultJson = MakeShared<FJsonObject>();
			ResultJson->SetStringField(TEXT("message"), TEXT("pong"));
			ResultJson->SetBoolField(TEXT("success"), true);
		}
		else if (CommandType == TEXT("execute_python"))
		{
			ResultJson = PythonExecutorHandler->ExecutePython(Params);
		}
		else if (CommandType == TEXT("spawn_actor") ||
				 CommandType == TEXT("list_level_actors") ||
				 CommandType == TEXT("delete_actor") ||
				 CommandType == TEXT("set_actor_transform") ||
				 CommandType == TEXT("get_actor_properties") ||
				 CommandType == TEXT("set_actor_property") ||
				 CommandType == TEXT("spawn_blueprint_actor") ||
				 CommandType == TEXT("create_material") ||
				 CommandType == TEXT("search_actors") ||
				 CommandType == TEXT("apply_material_to_actor") ||
				 CommandType == TEXT("get_actor_material_info") ||
				 CommandType == TEXT("search_assets") ||
				 CommandType == TEXT("list_folder_assets") ||
				 CommandType == TEXT("get_world_partition_info") ||
				 CommandType == TEXT("search_actors_in_region") ||
				 CommandType == TEXT("load_actor_by_guid") ||
				 CommandType == TEXT("set_region_loaded") ||
				 CommandType == TEXT("list_level_instances") ||
				 CommandType == TEXT("get_level_instance_actors") ||
				 CommandType == TEXT("list_gameplay_tags"))
		{
			ResultJson = EditorCommandHandler->HandleCommand(CommandType, Params);
		}
		else if (CommandType == TEXT("create_blueprint") ||
				 CommandType == TEXT("add_component_to_blueprint") ||
				 CommandType == TEXT("set_component_property") ||
				 CommandType == TEXT("set_physics_properties") ||
				 CommandType == TEXT("compile_blueprint") ||
				 CommandType == TEXT("set_mesh_material_color") ||
				 CommandType == TEXT("connect_blueprint_nodes") ||
				 CommandType == TEXT("add_component_getter_node") ||
				 CommandType == TEXT("add_blueprint_event_node") ||
				 CommandType == TEXT("add_custom_event_node") ||
				 CommandType == TEXT("add_blueprint_function_node") ||
				 CommandType == TEXT("add_blueprint_variable") ||
				 CommandType == TEXT("add_blueprint_input_action_node") ||
				 CommandType == TEXT("add_blueprint_self_reference") ||
				 CommandType == TEXT("list_blueprint_nodes") ||
				 CommandType == TEXT("apply_material_to_blueprint") ||
				 CommandType == TEXT("get_blueprint_material_info") ||
				 CommandType == TEXT("add_comment_box") ||
				 CommandType == TEXT("analyze_blueprint") ||
				 CommandType == TEXT("add_blueprint_flow_control_node") ||
				 CommandType == TEXT("set_pin_default_value") ||
				 CommandType == TEXT("add_blueprint_variable_node") ||
				 CommandType == TEXT("create_gameplay_effect") ||
				 CommandType == TEXT("create_gameplay_ability") ||
				 CommandType == TEXT("list_attribute_sets") ||
				 CommandType == TEXT("get_attribute_set_info") ||
				 CommandType == TEXT("search_functions") ||
				 CommandType == TEXT("get_class_functions") ||
				 CommandType == TEXT("add_function_override") ||
				 CommandType == TEXT("add_ability_task_node") ||
				 CommandType == TEXT("add_blueprint_generic_node") ||
				 CommandType == TEXT("set_node_property") ||
				 CommandType == TEXT("connect_nodes") ||
				 CommandType == TEXT("list_graphs") ||
				 CommandType == TEXT("create_child_blueprint") ||
				 CommandType == TEXT("build_ability_graph") ||
				 CommandType == TEXT("delete_blueprint_node") ||
				 CommandType == TEXT("delete_blueprint_variable") ||
				 CommandType == TEXT("delete_component_from_blueprint") ||
				 CommandType == TEXT("disconnect_blueprint_nodes") ||
				 CommandType == TEXT("add_pin") ||
				 CommandType == TEXT("delete_pin") ||
				 CommandType == TEXT("get_class_properties") ||
				 CommandType == TEXT("get_blueprint_variables") ||
				 CommandType == TEXT("add_property_get_set_node") ||
				 CommandType == TEXT("get_pin_value"))
		{
			ResultJson = BlueprintCommandHandler->HandleCommand(CommandType, Params);
		}
		// PCG commands
		else if (CommandType == TEXT("create_pcg_graph") ||
				 CommandType == TEXT("analyze_pcg_graph") ||
				 CommandType == TEXT("set_pcg_graph_to_component") ||
				 CommandType == TEXT("add_pcg_sampler_node") ||
				 CommandType == TEXT("add_pcg_filter_node") ||
				 CommandType == TEXT("add_pcg_transform_node") ||
				 CommandType == TEXT("add_pcg_spawner_node") ||
				 CommandType == TEXT("add_pcg_attribute_node") ||
				 CommandType == TEXT("add_pcg_flow_control_node") ||
				 CommandType == TEXT("add_pcg_generic_node") ||
				 CommandType == TEXT("list_pcg_nodes") ||
				 CommandType == TEXT("connect_pcg_nodes") ||
				 CommandType == TEXT("disconnect_pcg_nodes") ||
				 CommandType == TEXT("delete_pcg_node"))
		{
			ResultJson = PCGCommandHandler->HandleCommand(CommandType, Params);
		}
		else
		{
			ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
			ResponseJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown command: %s"), *CommandType));

			FString ResultString;
			TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
				TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ResultString);
			FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
			return ResultString;
		}

		bool bSuccess = true;
		FString ErrorMessage;

		if (ResultJson->HasField(TEXT("success")))
		{
			bSuccess = ResultJson->GetBoolField(TEXT("success"));
			if (!bSuccess && ResultJson->HasField(TEXT("error")))
			{
				ErrorMessage = ResultJson->GetStringField(TEXT("error"));
			}
		}

		if (bSuccess)
		{
			ResponseJson->SetStringField(TEXT("status"), TEXT("success"));
			ResponseJson->SetObjectField(TEXT("result"), ResultJson);
		}
		else
		{
			ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
			ResponseJson->SetStringField(TEXT("error"), ErrorMessage);
		}
	}
	catch (const std::exception& e)
	{
		ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
		ResponseJson->SetStringField(TEXT("error"), UTF8_TO_TCHAR(e.what()));
	}

	FString ResultString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ResultString);
	FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
	return ResultString;
}

void UUnrealEngineMCPBridge::StartServer()
{
	if (bIsRunning)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnrealEngineMCPBridge: Server is already running"));
		return;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealEngineMCPBridge: Failed to get socket subsystem"));
		return;
	}

	TSharedPtr<FSocket> NewListenerSocket = TSharedPtr<FSocket>(
		SocketSubsystem->CreateSocket(NAME_Stream, TEXT("UnrealEngineMCPListener"), false)
	);

	if (!NewListenerSocket.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealEngineMCPBridge: Failed to create listener socket"));
		return;
	}

	NewListenerSocket->SetReuseAddr(true);
	NewListenerSocket->SetNonBlocking(true);

	FIPv4Endpoint Endpoint(ServerAddress, Port);
	if (!NewListenerSocket->Bind(*Endpoint.ToInternetAddr()))
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealEngineMCPBridge: Failed to bind to %s:%d"),
			   *ServerAddress.ToString(), Port);
		return;
	}

	if (!NewListenerSocket->Listen(5))
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealEngineMCPBridge: Failed to start listening"));
		return;
	}

	ListenerSocket = NewListenerSocket;
	bIsRunning = true;

	UE_LOG(LogTemp, Display, TEXT("UnrealEngineMCPBridge: Server started on %s:%d"),
		   *ServerAddress.ToString(), Port);

	ServerThread = FRunnableThread::Create(
		new FUnrealEngineMCPRunnable(this, ListenerSocket),
		TEXT("UnrealEngineMCPServerThread"),
		0,
		TPri_Normal
	);

	if (!ServerThread)
	{
		UE_LOG(LogTemp, Error, TEXT("UnrealEngineMCPBridge: Failed to create server thread"));
		StopServer();
		return;
	}
}

void UUnrealEngineMCPBridge::StopServer()
{
	if (!bIsRunning)
	{
		return;
	}

	bIsRunning = false;

	if (ServerThread)
	{
		ServerThread->Kill(true);
		delete ServerThread;
		ServerThread = nullptr;
	}

	if (ListenerSocket.IsValid())
	{
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket.Get());
		ListenerSocket.Reset();
	}

	// Clear queues
	FMcpCommandRequest DummyRequest;
	while (CommandQueue.Dequeue(DummyRequest)) {}

	{
		FScopeLock Lock(&ResponseMapLock);
		ResponseMap.Empty();
	}

	UE_LOG(LogTemp, Display, TEXT("UnrealEngineMCPBridge: Server stopped"));
}
