#pragma once

#include "ModuleManager.h"
#include "Runtime/Core/Public/Containers/UnrealString.h"

class ILogCollector : public IModuleInterface
{
public:

	static inline ILogCollector& Get()
	{
		return FModuleManager::LoadModuleChecked<ILogCollector>("LogCollector");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("LogCollector");
	}

	
	virtual void LogRecord(int Severity, const char* FuncName, const TCHAR* Text, size_t Line = 0) = 0;

private:
};
