#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles Python code execution in Unreal Engine
 * This is the key handler for RAG-generated code
 */
class UNREALENGINEMCP_API FPythonExecutor
{
public:
	FPythonExecutor();
	~FPythonExecutor();

	/**
	 * Execute Python code in Unreal's Python environment
	 * Used by RAG tool to run generated code
	 */
	TSharedPtr<FJsonObject> ExecutePython(const TSharedPtr<FJsonObject>& Params);

private:
	/**
	 * Validate Python code for safety
	 * Checks for dangerous operations
	 */
	bool ValidateCode(const FString& Code, FString& OutError);

	/**
	 * Execute code with output capture
	 */
	bool ExecuteWithCapture(const FString& Code, FString& OutOutput, FString& OutError);
};