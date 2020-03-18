#pragma once

#include "ModuleManager.h"
#include "Core.h"
#include "ILogCollector.h"
#include "JsonUtilities.h"
// #include "Runtime/Core/Public/HAL/PlatformFilemanager.h"

DECLARE_LOG_CATEGORY_EXTERN(TAROT_LOG, Log, All);

typedef bool(*_getLogInit)(const char* maxSeverity, const char* fileName, size_t maxFileSize, int maxFile);
typedef void(*_getLogLevel)(int severity, const char* func, size_t line, const TCHAR* text);

enum LogSeverity
{
	none = 0,
	fatal = 1,
	error = 2,
	warning = 3,
	info = 4,
	debug = 5,
	verbose = 6
};

struct LogCollectorConfig
{
	bool activation;
	FString severity;
	FString logPath;
	uint32 fileSize;
	uint32 fileNum;

	LogCollectorConfig() :
		activation(true),
		severity("debug"),
		logPath("Logs/common/log.txt"),
		fileSize(1000000),
		fileNum(10){}
};

class FLogCollectorModule final : public ILogCollector
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline bool VerifyOrCreateDirectory(const FString& DirPath)
	{
		// If the path have file name and extension.
		FString path = DirPath;
		if (DirPath.Contains(TEXT(".")))
		{
			TArray<FString> pathArr;
			path.ParseIntoArray(pathArr, TEXT("/"), true);
			path.Empty();

			for (auto p : pathArr) {
				if (p.Contains(TEXT("."))) break;

				path += (p + TEXT("/"));
			}
		}
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		if (!PlatformFile.DirectoryExists(*path))
		{
			PlatformFile.CreateDirectoryTree(*path);
			if (!PlatformFile.DirectoryExists(*path)) {
				return false;
			}
			else {
				return true;
			}
		}
		return true;
	}


private:
	void InitLogCollectorLibrary();
	void LoadConfig(LogCollectorConfig& config);
	void LogRecord(int Severity, const char* FuncName, const TCHAR* Text, size_t Line = 0) override;

	void DebugMsg(FString& msg);

private:
	bool isValid;
	/** Handle to the third party dll we will load */
	void*	LogCollectorLibraryHandle;
	_getLogInit m_getLogInitFromDll;
	_getLogLevel m_getLogLevelFromDll;

	const FString LogInitProc = "InitLogSystem";
    const FString LogLevelProc = "ExportLogLevel";

	FString LogBasePath;

	LogCollectorConfig logCollectorConfig;
};
