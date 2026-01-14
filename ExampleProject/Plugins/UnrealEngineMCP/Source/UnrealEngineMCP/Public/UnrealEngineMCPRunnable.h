#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Sockets.h"

class UUnrealEngineMCPBridge;

/**
 * Runnable class for the MCP server thread
 * Handles incoming socket connections from Python MCP Server
 */
class FUnrealEngineMCPRunnable : public FRunnable
{
public:
	FUnrealEngineMCPRunnable(UUnrealEngineMCPBridge* InBridge, TSharedPtr<FSocket> InListenerSocket);
	virtual ~FUnrealEngineMCPRunnable();

	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

protected:
	void HandleClientConnection(TSharedPtr<FSocket> ClientSocket);
	bool ProcessMessageBuffer(TSharedPtr<FSocket> ClientSocket, FString& MessageBuffer);

private:
	UUnrealEngineMCPBridge* Bridge;
	TSharedPtr<FSocket> ListenerSocket;
	TSharedPtr<FSocket> ClientSocket;
	bool bRunning;
};