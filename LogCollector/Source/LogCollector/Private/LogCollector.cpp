// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LogCollector.h"
#include "Core.h"
#include "ModuleManager.h"
#include "IPluginManager.h"

#define LOCTEXT_NAMESPACE "FLogCollectorModule"

DEFINE_LOG_CATEGORY(TAROT_LOG);

void FLogCollectorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	isValid = true;

	// Get the base directory of this plugin
	FString BaseDir = IPluginManager::Get().FindPlugin("LogCollector")->GetBaseDir();

	// Add on the relative location of the third party dll and load it
	FString LibraryPath;
#if PLATFORM_WINDOWS
	// Get third party dll 
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/LogCollectorLibrary/Win64/LogSystemDLL.dll"));
#elif PLATFORM_MAC
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/LogCollectorLibrary/Mac/Release/libExampleLibrary.dylib"));
#endif // PLATFORM_WINDOWS

	LogCollectorLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;

	if (LogCollectorLibraryHandle)
	{
		// Get the exported functions
		m_getLogInitFromDll = (_getLogInit)FPlatformProcess::GetDllExport(
			LogCollectorLibraryHandle,
			*(LogInitProc));
		m_getLogLevelFromDll = (_getLogLevel)FPlatformProcess::GetDllExport(
			LogCollectorLibraryHandle,
			*(LogLevelProc));

		if (!m_getLogLevelFromDll)	// ?????????
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				LOCTEXT("ThirdPartyLibraryError", "Failed to get exported Log Level functions in Log Collector Library."));
			isValid = false;
		}
		if (!m_getLogInitFromDll)	// ?????????
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				LOCTEXT("ThirdPartyLibraryError", "Failed to get exported Log Inilization functions in Log Collector Library."));
			isValid = false;
		}
		
		LoadConfig(logCollectorConfig);
		InitLogCollectorLibrary();
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ThirdPartyLibraryError", "Failed to load Log Collector Library."));
		isValid = false;
	}
}

void FLogCollectorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Free the dll handle
	FPlatformProcess::FreeDllHandle(LogCollectorLibraryHandle);
	LogCollectorLibraryHandle = nullptr;
}

void FLogCollectorModule::InitLogCollectorLibrary()
{
#if UE_BUILD_SHIPPING
	// Get //InstallDir/WindowsNoEditor/ directory       Need to check!!!!!!
	LogBasePath = FPaths::ConvertRelativePathToFull(FPaths::RootDir());
#else
	LogBasePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*(FPaths::ProjectDir()));
#endif
	FString outlogPath;
	if (logCollectorConfig.logPath.Contains(TEXT(":"))) 
	{
		// Use full path from config file?
		outlogPath = logCollectorConfig.logPath;
	}
	else 
	{
		// Use the project path
		outlogPath = LogBasePath + logCollectorConfig.logPath;
	}
	// outlogPath = LogBasePath + "Logs/common";
	 
	if (!VerifyOrCreateDirectory(outlogPath))
	{
		// const FText dirCheckResult = FText::Format(LOCTEXT("ThirdPartyLibraryError", " LogBasePath is {0}"), result);
		FMessageDialog::Open(EAppMsgType::Ok,LOCTEXT("ThirdPartyLibraryError", "Log directory initialization failed. Logs can not output!"));
		return;
	}
	
	// Change log file path from "/" to "\\", need to repair this in fulture.
	FString outlogPathWithBackSlash = outlogPath.Replace(TEXT("/"), TEXT("\\"));
	// outlogPathWithBackSlash = TEXT("c:\\logs\\log.txt");
	// DebugMsg(outlogPathWithBackSlash);

	// const char* severity = *(logCollectorConfig.severity);
	// I forgot the "none" log level is not working...
	m_getLogInitFromDll(
		TCHAR_TO_ANSI(*logCollectorConfig.severity),
		TCHAR_TO_ANSI(*outlogPathWithBackSlash),
		logCollectorConfig.fileSize,
		logCollectorConfig.fileNum
	);
	
}

void FLogCollectorModule::LoadConfig(LogCollectorConfig & config)
{
	const FString JsonFilePath = FPaths::ProjectContentDir() + TEXT( "Config/config.json" );
	FString JsonString;

	FFileHelper::LoadFileToString(JsonString, *JsonFilePath);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		TSharedPtr<FJsonObject> LogObject = JsonObject->GetObjectField("Logging");
		config.activation = LogObject->GetBoolField("Activation");
		config.severity = LogObject->GetStringField("Severity");
		config.logPath = LogObject->GetStringField("LogPath");
		config.fileSize = LogObject->GetNumberField("FileSize");
		config.fileNum = LogObject->GetNumberField("FileNum");
	}
	else 
	{
		// delete
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ThirdPartyLibraryError", "Failed to load the LogCollector Library config file."));
	}
}

void FLogCollectorModule::LogRecord(int Severity, const char * FuncName, const TCHAR * Text, size_t Line)
{
	if (!isValid) return;

	if (logCollectorConfig.activation) 
	{
		m_getLogLevelFromDll(Severity, FuncName, Line, Text);
	}
	else 
	{
#if !UE_BUILD_SHIPPING
		// time, severity, func, message
		FString t_msg = FDateTime::Now().ToString() + " ";

		FString t_level = "";
		switch (Severity) {
		case 1:
			t_level = "FATAL   ";
			break;
		case 2:
			t_level = "ERROR   ";
			break;
		case 3:
			t_level = "WARNING ";
			break;
		case 4:
			t_level = "INFO    ";
			break;
		case 5:
			t_level = "DEBUG   ";
			break;
		case 6:
			t_level = "VERBOSE ";
			break;
		default:
			t_level = "NONE    ";
		}
		FString msg = t_msg + t_level + "["
			+ FuncName + "@" + FString::FromInt(int(Line)) + "] " + Text;
		UE_LOG(TAROT_LOG, Log, TEXT("%s"), *msg);
#endif
	}
}

void FLogCollectorModule::DebugMsg(FString & msg)
{
	const FText MessageText = FText::Format(
		LOCTEXT("ThirdPartyLibraryError", "Ready to init log collector. LogPath is {0}"),
		FText::FromString(msg)
	);
	FMessageDialog::Open(EAppMsgType::Ok, MessageText);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLogCollectorModule, LogCollector)
