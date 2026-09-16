using UnrealBuildTool;
using System.IO;

// The UE5 client links the engine-agnostic simulation as a static library built by CMake (Release, /MD),
// so the sim compiles once with its own flags and the game module only ever talks to gl::Game.
public class Gridlock : ModuleRules
{
	public Gridlock(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent" });

		string Root = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", ".."));
		PublicIncludePaths.Add(Path.Combine(Root, "sim", "include"));
		string Lib = Path.Combine(Root, "build", "gridlock_sim.lib");
		if (!File.Exists(Lib))
		{
			throw new BuildException("gridlock_sim.lib not found at " + Lib + " - run build.bat at the repository root first.");
		}
		PublicAdditionalLibraries.Add(Lib);
	}
}
