// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class LogCollectorLibrary : ModuleRules
{
	public LogCollectorLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

        string thirdPartyBinaryPath = Path.Combine(ModuleDirectory, 
            "..",
            "..",
            "..",
            "Binaries",
            "ThirdParty",
            "LogCollectorLibrary",
            "Win64");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
			PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "x64", "Release"));
			PublicAdditionalLibraries.Add("LogSystemDLL.lib");

            string logCollectorPath = Path.Combine(ModuleDirectory,
                "x64",
                "Release",
                "LogSystemDLL.dll");
            string logCollectorBinaryPath = Path.Combine(ModuleDirectory,
                "..",
                "..",
                "..",
                "Binaries",
                "ThirdParty",
                "LogCollectorLibrary",
                "Win64",
                "LogSystemDLL.dll");

            System.IO.Directory.CreateDirectory(thirdPartyBinaryPath);
            CopyFile(logCollectorPath, logCollectorBinaryPath);
            RuntimeDependencies.Add(logCollectorBinaryPath);
			
			// Delay-load the DLL, so we can load it from the right place first
			PublicDelayLoadDLLs.Add("LogSystemDLL.dll");
		}
        /* // Now only for windows
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory,
                "Mac",
                "Release",
                "libExampleLibrary.dylib"));
		}*/
	}

    private void CopyFile(string src, string dest)
    {
        if (System.IO.File.Exists(dest))
        {
            System.IO.File.SetAttributes(dest,
                System.IO.File.GetAttributes(dest) & ~System.IO.FileAttributes.ReadOnly);
        }

        try
        {
            System.IO.File.Copy(src, dest, true);
        }
        catch(System.Exception e)
        {
            System.Console.WriteLine("Failed to copy file {0}", e.Message);
        }
    }
}
