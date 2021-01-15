// NetImgui.sharpmake.cs
using Sharpmake;
using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;
using System.IO;
using System.Text.RegularExpressions;
using ProjConfig = Sharpmake.Project.Configuration;

namespace NetImgui
{	
	//=============================================================================================
	// CUSTOM PROPERTIES
	//=============================================================================================
	// Target Compiler options
	[Fragment, Flags]
	public enum Compiler
	{
		VS = 1 << 1,
		Clang = 1 << 2
	}

	// Target Config
	public class NetImguiTarget : ITarget
	{
		// DevEnv and Platform are mandatory on all targets so we define them.
		public DevEnv DevEnv;
		public Platform Platform;
		public Compiler Compiler;
		public Optimization Optimization;

		public NetImguiTarget()
		{
			DevEnv = DevEnv.vs2019;
			Platform = Platform.win64;
			Optimization = Optimization.Debug | Optimization.Release;
			Compiler = Compiler.VS | Compiler.Clang;
		}
		
		static public string GetPath(string pathFromRoot)
		{
			return sRootPath + pathFromRoot;
		}
		static protected string sRootPath = AppDomain.CurrentDomain.BaseDirectory + @"..\..";
	}

	//=============================================================================================
	// SHADERS COMPILATION TASK
	//=============================================================================================
	public class CustomBuildFileHLSL : ProjConfig.CustomFileBuildStep
	{
		public enum ShaderProfile
		{ 
			ps_5_0,
			vs_5_0,
			cs_5_0,
		}

		public CustomBuildFileHLSL(ProjConfig conf, string targetName, ShaderProfile shaderProfile, string entryPoint, string outputDir, string sourceFile)
		{			
			KeyInput			= sourceFile;
			string resourceName = Path.GetFileNameWithoutExtension(sourceFile);

			Output				= string.Format(@"{0}\{1}.h", outputDir, resourceName); 
			Description			= "Shader (" + shaderProfile.ToString() + ") : " + sourceFile;
			Executable			= ""; // Setting 'fxc' exe in ExeArgs instead, avoid build system adding a uneeded dependency to 'fxc'
			ExecutableArguments = string.Format("fxc /Zi /nologo /O2 /E\"{0}\" /T {1} /Fh\"{2}\" /Vn\"gpShader_{3}\" \"{4}\"", entryPoint, shaderProfile.ToString(), Output, resourceName, sourceFile);
		}

		public static void AddFilesExt(Project project)
		{
			project.SourceFilesExtensions.Add(".hlsl");
		}

		public static void ClaimAllShaderFiles(Project project)
		{			
			ClaimShaderFiles(project, "VS.hlsl", ShaderProfile.vs_5_0, "main");
			ClaimShaderFiles(project, "PS.hlsl", ShaderProfile.ps_5_0, "main");
			ClaimShaderFiles(project, "CS.hlsl", ShaderProfile.cs_5_0, "main");			
		}

		public static void ClaimShaderFiles(Project project, string filenameEnding, ShaderProfile shaderProfile, string entryName)
		{
			Strings hlsl_Files = new Strings(project.ResolvedSourceFiles.Where(file => file.EndsWith(filenameEnding, StringComparison.InvariantCultureIgnoreCase)));
			if (hlsl_Files.Count() > 0)
			{
				foreach (ProjConfig conf in project.Configurations)
				{
					string targetName = conf.Target.Name;
					string outputDir = string.Format(@"{0}\{1}_{2}\", NetImguiTarget.GetPath(@"\_generated\Shaders"), project.Name, conf.Target.GetOptimization());
					conf.IncludePaths.Add(outputDir);
					foreach (string file in hlsl_Files)
					{
						CustomBuildFileHLSL HlslCompileTask = new CustomBuildFileHLSL(conf, targetName, shaderProfile, entryName, outputDir, Project.GetCapitalizedFile(file));
						project.ResolvedSourceFiles.Add(HlslCompileTask.Output);
						conf.CustomFileBuildSteps.Add(HlslCompileTask);
					}
				}
			}
		}
	}	
	
	//=============================================================================================
	// PROJECTS
	//=============================================================================================
	[Sharpmake.Generate]
    public class ProjectBase : Project
	{
		public ProjectBase(bool isExe)
		: base(typeof(NetImguiTarget))
		{
			AddTargets(new NetImguiTarget{});
			AddTargets(new NetImguiTarget{DevEnv = DevEnv.vs2017, Compiler = Compiler.VS});
			
			CustomBuildFileHLSL.AddFilesExt(this);
			IsFileNameToLower		= false;
			IsTargetFileNameToLower = false;
			mIsExe					= isExe;
		}

		protected override void ExcludeOutputFiles()
		{
			base.ExcludeOutputFiles();
			CustomBuildFileHLSL.ClaimAllShaderFiles(this);			
		}
		
		[Configure()]
        public void ConfigureAll(Configuration conf, NetImguiTarget target)
        {			
			conf.Name				= @"[target.Compiler]_[target.Optimization]";
			conf.ProjectFileName	= @"[project.Name]";
			conf.TargetFileSuffix	= @"_[target.Optimization]";
			conf.ProjectPath		= NetImguiTarget.GetPath(@"\_projects\[target.DevEnv]");
			conf.TargetPath			= NetImguiTarget.GetPath( mIsExe	? @"\_Bin\[target.DevEnv]_[target.Compiler]_[target.Platform]" 
																		: @"\_generated\Libs\[target.DevEnv]_[target.Compiler]_[target.Platform]");
			conf.IntermediatePath	= NetImguiTarget.GetPath(@"\_intermediate\[target.DevEnv]_[target.Compiler]_[target.Platform]_[target.Optimization]\[project.Name]");
			conf.Output				= mIsExe ? Project.Configuration.OutputType.Exe : Project.Configuration.OutputType.Lib;

			conf.IncludePaths.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends");
			
			if ( target.Compiler == Compiler.Clang )
				conf.Options.Add(Options.Vc.General.PlatformToolset.ClangCL);

			if (mIsExe)
			{
				conf.VcxprojUserFile = new ProjConfig.VcxprojUserFileSettings();
				conf.VcxprojUserFile.LocalDebuggerWorkingDirectory = "$(TargetDir)";
				conf.Options.Add(Options.Vc.Linker.SubSystem.Application);
				conf.LibraryFiles.Add("D3D11.lib");
				conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code"));
			}

			conf.Options.Add(Options.Vc.General.WindowsTargetPlatformVersion.Latest);
			conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);
			conf.Options.Add(Options.Vc.General.CharacterSet.Unicode);			
			conf.Options.Add(Options.Vc.Linker.TreatLinkerWarningAsErrors.Enable);
			
			conf.Defines.Add("_HAS_EXCEPTIONS=0"); // Prevents error in VisualStudio c++ library with NoExcept, like xlocale
			
			//conf.Options.Add(new Options.Vc.Compiler.DisableSpecificWarnings(""));
			//conf.Options.Add(Options.Vc.Librarian.TreatLibWarningAsErrors.Enable);	//Note: VisualStudio 2019 doesn't support this option properly
		}
		
		// Add sources files for platform specific Dear ImGui backend (OS/Renderer support)
		public void AddImguiBackendSources()
		{
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_dx11.h");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_dx11.cpp");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_win32.h");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_win32.cpp");
		}
		bool mIsExe;
	}

	[Sharpmake.Generate]
	public class ProjectImgui : ProjectBase
	{
		public ProjectImgui()
		: base(false)
		{
			Name = "DearImgui";
			SourceRootPath = NetImguiTarget.GetPath(sDefaultPath);
			SourceFilesExcludeRegex.Add(@"backends\.*");
		}
		public static string sDefaultPath = @"\Code\ThirdParty\DearImgui";
	}

	[Sharpmake.Generate]
    public class ProjectNetImgui : ProjectBase
    {
		public ProjectNetImgui(string imguiFullPath)
		: base(false)
        {
			mVersion = Path.GetFileName(imguiFullPath);
			mImguiPath = imguiFullPath;
			Name = "NetImguiLib (" + mVersion + ")";
            SourceRootPath = NetImguiTarget.GetPath(@"\Code\Client");
			SourceFiles.Add(mImguiPath + @"\imgui.h");
        }

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.Options.Add(Options.Vc.General.WarningLevel.EnableAllWarnings);
			conf.IncludePaths.Add(mImguiPath);
			conf.LibraryFiles.Add("ws2_32.lib");
        }
		string mVersion;
		string mImguiPath;
    }
	
	[Sharpmake.Generate]
    public class ProjectSample : ProjectBase
    {
        public ProjectSample(string inName)
		: base(true)
		{
            Name			= inName;
            SourceRootPath	= NetImguiTarget.GetPath(@"\Code\Sample\") + Name;
			AdditionalSourceRootPaths.Add(NetImguiTarget.GetPath(@"\Code\Sample\Common"));
			AddImguiBackendSources();
        }

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);			
			conf.AddPublicDependency<ProjectImgui>(target);
			conf.AddPublicDependency<ProjectNetImgui_Default>(target);
			conf.IncludePaths.Add(ProjectImgui.sDefaultPath);
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
		}
    }
	
	//=============================================================================================
	// SOLUTIONS
	//=============================================================================================
	public class SolutionBase : Sharpmake.Solution
    {
		public SolutionBase(string inName)
		: base(typeof(NetImguiTarget))
        {
			AddTargets(new NetImguiTarget{ });
			AddTargets(new NetImguiTarget{DevEnv = DevEnv.vs2017, Compiler = Compiler.VS});
			Name					= inName;
			IsFileNameToLower		= false;
		}
 
        [Configure()]
        public void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			conf.Name				= "[target.Compiler]_[target.Optimization]";
            conf.SolutionFileName	= "[target.DevEnv]_[solution.Name]";
            conf.SolutionPath		= NetImguiTarget.GetPath(@"\_projects");
		}
	}
}