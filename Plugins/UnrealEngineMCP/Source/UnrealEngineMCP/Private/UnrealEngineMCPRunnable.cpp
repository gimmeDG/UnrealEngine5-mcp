#include "UnrealEngineMCPRunnable.h"
#include "UnrealEngineMCPBridge.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "HAL/PlatformTime.h"

#define MCP_RESPONSE_TIMEOUT 60.0f
#define MCP_RECV_BUFFER_SIZE 65536

FUnrealEngineMCPRunnable::FUnrealEngineMCPRunnable(
	UUnrealEngineMCPBridge* InBridge,
	TSharedPtr<FSocket> InListenerSocket)
	: Bridge(InBridge)
	, ListenerSocket(InListenerSocket)
	, bRunning(true)
{
	UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Created with Command Queue support"));
}

FUnrealEngineMCPRunnable::~FUnrealEngineMCPRunnable()
{
}

bool FUnrealEngineMCPRunnable::Init()
{
	UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Initialized"));
	return true;
}

uint32 FUnrealEngineMCPRunnable::Run()
{
	UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Server thread starting..."));

	while (bRunning)
	{
		bool bPending = false;
		if (ListenerSocket->HasPendingConnection(bPending) && bPending)
		{
			UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Client connection pending, accepting..."));

			ClientSocket = TSharedPtr<FSocket>(ListenerSocket->Accept(TEXT("MCPClient")));
			if (ClientSocket.IsValid())
			{
				UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Client connection accepted"));

				ClientSocket->SetNoDelay(true);
				int32 SocketBufferSize = MCP_RECV_BUFFER_SIZE;
				ClientSocket->SetSendBufferSize(SocketBufferSize, SocketBufferSize);
				ClientSocket->SetReceiveBufferSize(SocketBufferSize, SocketBufferSize);

				HandleClientConnection(ClientSocket);

				ClientSocket->Close();
				ClientSocket.Reset();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("FUnrealEngineMCPRunnable: Failed to accept client connection"));
			}
		}

		FPlatformProcess::Sleep(0.01f);  // 10ms - faster response than before
	}

	UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Server thread stopping"));
	return 0;
}

void FUnrealEngineMCPRunnable::Stop()
{
	bRunning = false;
}

void FUnrealEngineMCPRunnable::Exit()
{
	UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Thread exiting"));
}

void FUnrealEngineMCPRunnable::HandleClientConnection(TSharedPtr<FSocket> InClientSocket)
{
	if (!InClientSocket.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("FUnrealEngineMCPRunnable: Invalid client socket"));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Handling client connection"));

	const int32 MaxBufferSize = MCP_RECV_BUFFER_SIZE;
	uint8 Buffer[MCP_RECV_BUFFER_SIZE];
	FString MessageBuffer;

	while (bRunning && InClientSocket.IsValid())
	{
		int32 BytesRead = 0;
		if (InClientSocket->Recv(Buffer, MaxBufferSize - 1, BytesRead))
		{
			if (BytesRead == 0)
			{
				UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Client disconnected (zero bytes)"));
				break;
			}

			Buffer[BytesRead] = '\0';
			FString ReceivedText = UTF8_TO_TCHAR(Buffer);
			MessageBuffer.Append(ReceivedText);

			if (!ProcessMessageBuffer(InClientSocket, MessageBuffer))
			{
				break;
			}
		}
		else
		{
			int32 LastError = (int32)ISocketSubsystem::Get()->GetLastErrorCode();

			if (LastError == SE_EWOULDBLOCK)
			{
				FPlatformProcess::Sleep(0.001f);
				continue;
			}
			else if (LastError == SE_EINTR)
			{
				continue;
			}
			else if (LastError == SE_NO_ERROR)
			{
				break;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("FUnrealEngineMCPRunnable: Socket error: %d"), LastError);
				break;
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Client connection handler exiting"));
}

bool FUnrealEngineMCPRunnable::ProcessMessageBuffer(TSharedPtr<FSocket> InClientSocket, FString& MessageBuffer)
{
	int32 NewlineIndex;
	while (MessageBuffer.FindChar(TEXT('\n'), NewlineIndex))
	{
		FString CompleteMessage = MessageBuffer.Left(NewlineIndex);
		MessageBuffer = MessageBuffer.Mid(NewlineIndex + 1);

		CompleteMessage.TrimStartAndEndInline();

		if (CompleteMessage.IsEmpty())
		{
			continue;
		}

		UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Processing message: %s"),
			   *CompleteMessage.Left(200));

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CompleteMessage);

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			FString CommandType;
			if (JsonObject->TryGetStringField(TEXT("type"), CommandType))
			{
				TSharedPtr<FJsonObject> Params;
				if (JsonObject->HasField(TEXT("params")))
				{
					Params = JsonObject->GetObjectField(TEXT("params"));
				}
				else
				{
					Params = MakeShared<FJsonObject>();
				}

				uint32 RequestId;
				if (!Bridge->EnqueueCommand(CommandType, Params, RequestId))
				{
					FString BusyResponse = TEXT("{\"status\":\"error\",\"error\":\"Server busy, command queue full\"}\n");
					int32 BytesSent = 0;
					FTCHARToUTF8 UTF8Response(*BusyResponse);
					InClientSocket->Send((uint8*)UTF8Response.Get(), UTF8Response.Length(), BytesSent);
					return true;
				}

				FMcpCommandResponse Response;
				if (Bridge->WaitForResponse(RequestId, Response, MCP_RESPONSE_TIMEOUT))
				{
					FString ResponseStr = Response.Response + TEXT("\n");

					UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Sending response: %s"),
						   *ResponseStr.Left(200));

					int32 BytesSent = 0;
					FTCHARToUTF8 UTF8Response(*ResponseStr);
					if (!InClientSocket->Send((uint8*)UTF8Response.Get(), UTF8Response.Length(), BytesSent))
					{
						UE_LOG(LogTemp, Warning, TEXT("FUnrealEngineMCPRunnable: Failed to send response"));
						return false;
					}

					UE_LOG(LogTemp, Display, TEXT("FUnrealEngineMCPRunnable: Response sent, bytes: %d"), BytesSent);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("FUnrealEngineMCPRunnable: Response timeout for command: %s"), *CommandType);

					FString TimeoutResponse = TEXT("{\"status\":\"error\",\"error\":\"Command timeout\"}\n");
					int32 BytesSent = 0;
					FTCHARToUTF8 UTF8Response(*TimeoutResponse);
					InClientSocket->Send((uint8*)UTF8Response.Get(), UTF8Response.Length(), BytesSent);
					return false;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("FUnrealEngineMCPRunnable: Missing 'type' field"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("FUnrealEngineMCPRunnable: Failed to parse JSON: %s"),
				   *CompleteMessage.Left(200));
			return false;
		}
	}

	return true;
}
