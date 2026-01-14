#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Dom/JsonObject.h"
#include "Containers/Queue.h"
#include "HAL/CriticalSection.h"
#include "UnrealEngineMCPBridge.generated.h"

class FMcpServerRunnable;
class FEditorCommands;
class FBlueprintCommands;
class FPCGCommands;
class FPythonExecutor;

// Command request structure
struct FMcpCommandRequest
{
	uint32 RequestId;
	FString CommandType;
	TSharedPtr<FJsonObject> Params;
	double Timestamp;

	FMcpCommandRequest() : RequestId(0), Timestamp(0.0) {}
	FMcpCommandRequest(uint32 InId, const FString& InType, TSharedPtr<FJsonObject> InParams)
		: RequestId(InId), CommandType(InType), Params(InParams), Timestamp(FPlatformTime::Seconds()) {}
};

// Command response structure
struct FMcpCommandResponse
{
	uint32 RequestId;
	FString Response;
	bool bSuccess;

	FMcpCommandResponse() : RequestId(0), bSuccess(false) {}
	FMcpCommandResponse(uint32 InId, const FString& InResponse, bool InSuccess)
		: RequestId(InId), Response(InResponse), bSuccess(InSuccess) {}
};

/**
 * Editor subsystem for MCP Bridge
 * Uses Command Queue pattern for non-blocking network operations
 */
UCLASS()
class UNREALENGINEMCP_API UUnrealEngineMCPBridge : public UEditorSubsystem, public FTickableEditorObject
{
	GENERATED_BODY()

public:
	UUnrealEngineMCPBridge();
	virtual ~UUnrealEngineMCPBridge();

	// UEditorSubsystem implementation
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableEditorObject implementation
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bIsRunning; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UUnrealEngineMCPBridge, STATGROUP_Tickables); }

	// Server control
	void StartServer();
	void StopServer();
	bool IsRunning() const { return bIsRunning; }

	// Queue-based command handling (called from network thread)
	bool EnqueueCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params, uint32& OutRequestId);
	bool TryDequeueResponse(uint32 RequestId, FMcpCommandResponse& OutResponse);
	bool WaitForResponse(uint32 RequestId, FMcpCommandResponse& OutResponse, float TimeoutSeconds = 30.0f);
	int32 GetPendingCommandCount() const { return PendingCommandCount.Load(); }

private:
	// Process pending commands on Game Thread
	void ProcessCommandQueue();

	// Execute single command and generate response
	FString ExecuteCommandInternal(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	// Server state
	bool bIsRunning;
	TSharedPtr<FSocket> ListenerSocket;
	FRunnableThread* ServerThread;

	// Server configuration
	FIPv4Address ServerAddress;
	uint16 Port;

	// Command handlers
	TSharedPtr<FEditorCommands> EditorCommandHandler;
	TSharedPtr<FBlueprintCommands> BlueprintCommandHandler;
	TSharedPtr<FPCGCommands> PCGCommandHandler;
	TSharedPtr<FPythonExecutor> PythonExecutorHandler;

	// Command queues (thread-safe)
	TQueue<FMcpCommandRequest, EQueueMode::Mpsc> CommandQueue;  // Multi-producer, single-consumer
	TMap<uint32, FMcpCommandResponse> ResponseMap;
	FCriticalSection ResponseMapLock;

	// Request ID counter
	TAtomic<uint32> NextRequestId;

	// Queue size tracking for overflow protection
	TAtomic<int32> PendingCommandCount;
};
