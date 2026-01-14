#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FUnrealEngineMCPModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FUnrealEngineMCPModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FUnrealEngineMCPModule>("UnrealEngineMCP");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("UnrealEngineMCP");
	}
};