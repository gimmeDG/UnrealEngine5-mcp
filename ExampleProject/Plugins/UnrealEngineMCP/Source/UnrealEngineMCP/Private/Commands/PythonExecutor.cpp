#include "Commands/PythonExecutor.h"
#include "Commands/CommonUtils.h"
#include "Dom/JsonObject.h"
#include "IPythonScriptPlugin.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

FPythonExecutor::FPythonExecutor()
{
	UE_LOG(LogTemp, Display, TEXT("FPythonExecutor::FPythonExecutor: Initialized"));
}

FPythonExecutor::~FPythonExecutor()
{
}

TSharedPtr<FJsonObject> FPythonExecutor::ExecutePython(const TSharedPtr<FJsonObject>& Params)
{
	// Get script from params
	FString Script;
	if (!Params->TryGetStringField(TEXT("script"), Script))
	{
		return FCommonUtils::CreateErrorResponse(TEXT("Missing 'script' parameter"));
	}

	UE_LOG(LogTemp, Display, TEXT("FPythonExecutor::ExecutePython: Executing Python script (%d chars)"), Script.Len());

	// Validate code safety
	FString ValidationError;
	if (!ValidateCode(Script, ValidationError))
	{
		UE_LOG(LogTemp, Warning, TEXT("FPythonExecutor::ExecutePython: Code failed safety check: %s"), *ValidationError);
		return FCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Safety check failed: %s"), *ValidationError));
	}

	// Execute code
	FString Output, Error;
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();

	if (ExecuteWithCapture(Script, Output, Error))
	{
		ResultObj->SetBoolField(TEXT("success"), true);
		ResultObj->SetStringField(TEXT("output"), Output);

		if (!Error.IsEmpty())
		{
			ResultObj->SetStringField(TEXT("stderr"), Error);
		}

		UE_LOG(LogTemp, Display, TEXT("FPythonExecutor::ExecutePython: Execution succeeded"));
	}
	else
	{
		ResultObj->SetBoolField(TEXT("success"), false);
		ResultObj->SetStringField(TEXT("error"), Error);
		ResultObj->SetStringField(TEXT("output"), Output);

		UE_LOG(LogTemp, Display, TEXT("FPythonExecutor::ExecutePython: Execution failed: %s"), *Error);
	}

	return ResultObj;
}

bool FPythonExecutor::ValidateCode(const FString& Code, FString& OutError)
{
	// Check for dangerous patterns
	TArray<FString> DangerousKeywords = {
		TEXT("os.remove"),
		TEXT("os.unlink"),
		TEXT("shutil.rmtree"),
		TEXT("subprocess."),
		TEXT("__import__"),
		TEXT("eval("),
		TEXT("exec("),
		TEXT("compile("),
		TEXT("open(") // File operations
	};

	for (const FString& Keyword : DangerousKeywords)
	{
		if (Code.Contains(Keyword))
		{
			OutError = FString::Printf(TEXT("Dangerous operation detected: %s"), *Keyword);
			return false;
		}
	}

	// Check length
	if (Code.Len() > 5000)
	{
		OutError = TEXT("Script too long (max 5000 characters)");
		return false;
	}

	return true;
}

bool FPythonExecutor::ExecuteWithCapture(const FString& Code, FString& OutOutput, FString& OutError)
{
	// Get Python plugin
	IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
	if (!PythonPlugin)
	{
		OutError = TEXT("Python plugin not available");
		return false;
	}

	// Generate unique ID for temp files
	FString UniqueId = FGuid::NewGuid().ToString(EGuidFormats::Short);

	// Setup temp file paths
	FString TempDir = FPaths::ProjectSavedDir() / TEXT("MCP");
	IFileManager::Get().MakeDirectory(*TempDir, true);

	FString OutputFile = TempDir / FString::Printf(TEXT("stdout_%s.txt"), *UniqueId);
	FString ErrorFile = TempDir / FString::Printf(TEXT("stderr_%s.txt"), *UniqueId);
	FString SuccessFile = TempDir / FString::Printf(TEXT("success_%s.txt"), *UniqueId);

	// Escape backslashes for Python string
	FString OutputFilePy = OutputFile.Replace(TEXT("\\"), TEXT("\\\\"));
	FString ErrorFilePy = ErrorFile.Replace(TEXT("\\"), TEXT("\\\\"));
	FString SuccessFilePy = SuccessFile.Replace(TEXT("\\"), TEXT("\\\\"));

	// Wrap code to capture output and write to temp files
	FString WrappedCode = FString::Printf(TEXT(
		"import sys\n"
		"from io import StringIO\n"
		"import unreal\n"
		"\n"
		"_mcp_stdout = StringIO()\n"
		"_mcp_stderr = StringIO()\n"
		"_mcp_old_stdout = sys.stdout\n"
		"_mcp_old_stderr = sys.stderr\n"
		"sys.stdout = _mcp_stdout\n"
		"sys.stderr = _mcp_stderr\n"
		"_mcp_success = True\n"
		"\n"
		"try:\n"
		"    %s\n"
		"except Exception as _mcp_e:\n"
		"    import traceback\n"
		"    sys.stderr.write(str(_mcp_e) + '\\n')\n"
		"    sys.stderr.write(traceback.format_exc())\n"
		"    _mcp_success = False\n"
		"finally:\n"
		"    sys.stdout = _mcp_old_stdout\n"
		"    sys.stderr = _mcp_old_stderr\n"
		"    _mcp_output = _mcp_stdout.getvalue()\n"
		"    _mcp_error = _mcp_stderr.getvalue()\n"
		"    try:\n"
		"        with open('%s', 'w', encoding='utf-8') as _f:\n"
		"            _f.write(_mcp_output)\n"
		"        with open('%s', 'w', encoding='utf-8') as _f:\n"
		"            _f.write(_mcp_error)\n"
		"        with open('%s', 'w', encoding='utf-8') as _f:\n"
		"            _f.write('1' if _mcp_success else '0')\n"
		"    except Exception as _write_e:\n"
		"        unreal.log_error('MCP: Failed to write output: ' + str(_write_e))\n"
	), *Code.Replace(TEXT("\n"), TEXT("\n    ")), *OutputFilePy, *ErrorFilePy, *SuccessFilePy);

	// Execute wrapped code
	bool bExecSuccess = PythonPlugin->ExecPythonCommand(*WrappedCode);

	// Read captured output from temp files
	FString CapturedOutput, CapturedError, SuccessStr;
	bool bScriptSuccess = true;

	if (FFileHelper::LoadFileToString(CapturedOutput, *OutputFile))
	{
		OutOutput = CapturedOutput;
		IFileManager::Get().Delete(*OutputFile);
	}
	else
	{
		OutOutput = TEXT("");
	}

	if (FFileHelper::LoadFileToString(CapturedError, *ErrorFile))
	{
		OutError = CapturedError;
		IFileManager::Get().Delete(*ErrorFile);
	}
	else
	{
		OutError = TEXT("");
	}

	if (FFileHelper::LoadFileToString(SuccessStr, *SuccessFile))
	{
		bScriptSuccess = (SuccessStr.TrimStartAndEnd() == TEXT("1"));
		IFileManager::Get().Delete(*SuccessFile);
	}
	else
	{
		bScriptSuccess = bExecSuccess;
	}

	return bScriptSuccess;
}