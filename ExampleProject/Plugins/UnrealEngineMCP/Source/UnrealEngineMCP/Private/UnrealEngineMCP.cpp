#include "UnrealEngineMCP.h"
#include "UnrealEngineMCPBridge.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FUnrealEngineMCPModule"

void FUnrealEngineMCPModule::StartupModule()
{
	UE_LOG(LogTemp, Display, TEXT("Unreal Engine MCP Module: Starting up"));

	// Bridge will be auto-initialized as EditorSubsystem
}

void FUnrealEngineMCPModule::ShutdownModule()
{
	UE_LOG(LogTemp, Display, TEXT("Unreal Engine MCP Module: Shutting down"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealEngineMCPModule, UnrealEngineMCP)