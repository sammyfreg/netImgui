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
					string outputDir = string.Format(@"{0}\_Intermediate\_Shaders\{1}_{2}\", project.SharpmakeCsPath, project.Name, conf.Target.GetOptimization());
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
			//AddTargets(new NetImguiTarget{}, new NetImguiTarget{DevEnv=DevEnv.vs2017, Compiler = Compiler.VS});			
			AddTargets(new NetImguiTarget{});
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
			conf.ProjectPath		= @"[project.SharpmakeCsPath]\_Projects\[target.DevEnv]";
			conf.TargetPath			= mIsExe ? @"[project.SharpmakeCsPath]\_Bin\[target.Compiler]_[target.Platform]" : @"[project.SharpmakeCsPath]\_Lib\[target.Compiler]_[target.Platform]";
			conf.IntermediatePath	= @"[project.SharpmakeCsPath]\_Intermediate\[target.Compiler]_[target.Platform]_[target.Optimization]\[project.Name]";
			conf.Output				= mIsExe ? Project.Configuration.OutputType.Exe : Project.Configuration.OutputType.Lib;

			if ( target.Compiler == Compiler.Clang )
				conf.Options.Add(Options.Vc.General.PlatformToolset.LLVM);

			if (mIsExe)
			{
				conf.VcxprojUserFile = new ProjConfig.VcxprojUserFileSettings();
				conf.VcxprojUserFile.LocalDebuggerWorkingDirectory = "$(TargetDir)";
				conf.Options.Add(Options.Vc.Linker.SubSystem.Application);
				conf.LibraryFiles.Add("D3D11.lib");
				conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\Code");
			}

			conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);
			conf.Options.Add(Options.Vc.General.CharacterSet.Unicode);			
			conf.Options.Add(Options.Vc.Linker.TreatLinkerWarningAsErrors.Enable);
			//conf.Options.Add(new Options.Vc.Compiler.DisableSpecificWarnings(""));
			//conf.Options.Add(Options.Vc.Librarian.TreatLibWarningAsErrors.Enable);	//Note: VisualStudio 2019 doesn't support this option properly
		}
		bool mIsExe;
	}

	[Sharpmake.Generate]
	public class ProjectImgui : ProjectBase
	{
		public ProjectImgui(string version)
		: base(false)
		{
			Name = version;
			SourceRootPath = @"[project.SharpmakeCsPath]\..\Code\ThirdParty\" + version;
		}
	}

	[Sharpmake.Generate]
    public class ProjectNetImgui : ProjectBase
    {
		public ProjectNetImgui(string version)
		: base(false)
        {
			mVersion = version;
			Name = "netImguiLib (" + version + ")";
            SourceRootPath = @"[project.SharpmakeCsPath]\..\Code\Client";
			SourceFiles.Add(@"[project.SharpmakeCsPath]\..\Code\ThirdParty\" + SolutionAll.sDefaultImguiVersion + @"\imgui.h");
        }

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.Options.Add(Options.Vc.General.WarningLevel.EnableAllWarnings);
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\Code\ThirdParty\" + mVersion);
			conf.LibraryFiles.Add("ws2_32.lib");
        }
		string mVersion;
    }

	// Generate the default Imgui Library  (used to link against with Samples/Server)
	[Sharpmake.Generate] public class ProjectImgui_Default : ProjectImgui { public ProjectImgui_Default() : base(SolutionAll.sDefaultImguiVersion) {} }
	
	// Generate the default netImgui library (used to link against with Samples/Server)
	[Sharpmake.Generate] public class ProjectNetImgui_Default : ProjectNetImgui { 
		public ProjectNetImgui_Default() : base(SolutionAll.sDefaultImguiVersion) { Name = "netImguiLib"; } 
	}
	
	// Test compiling netImgui with the Disabled Define
	[Sharpmake.Generate] public class ProjectNetImgui_Disabled : ProjectNetImgui { 
		public ProjectNetImgui_Disabled() : base(SolutionAll.sDefaultImguiVersion) { Name = "netImguiLib (Disabled)"; }
		
		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.Defines.Add("NETIMGUI_ENABLED=0");
		}
	}
	
	// Try compiling netImgui against various version of Imgui releases
	[Sharpmake.Generate] public class ProjectNetImgui_17500 	: ProjectNetImgui { public ProjectNetImgui_17500() 		: base("Imgui_17500"){} }	
	[Sharpmake.Generate] public class ProjectNetImgui_17600 	: ProjectNetImgui { public ProjectNetImgui_17600() 		: base("Imgui_17600"){} }
	[Sharpmake.Generate] public class ProjectNetImgui_17700 	: ProjectNetImgui { public ProjectNetImgui_17700() 		: base("Imgui_17700"){} }
	[Sharpmake.Generate] public class ProjectNetImgui_17800 	: ProjectNetImgui { public ProjectNetImgui_17800()		: base("Imgui_17800"){} }
	[Sharpmake.Generate] public class ProjectNetImgui_Dock17601 : ProjectNetImgui { public ProjectNetImgui_Dock17601()	: base("Imgui_Dock17601") {} }
	
	[Sharpmake.Generate]
    public class ProjectNetImguiServer : ProjectBase
    {
        public ProjectNetImguiServer()
		: base(true)
		{
            Name			= "netImguiServer"; 
            SourceRootPath	= @"[project.SharpmakeCsPath]\..\Code\ServerApp";
			ResourceFiles.Add(@"[project.SharpmakeCsPath]\..\Code\ServerApp\small.ico");
			ResourceFiles.Add(@"[project.SharpmakeCsPath]\..\Code\ServerApp\netImguiApp.ico");
			ResourceFiles.Add(@"[project.SharpmakeCsPath]\..\Code\ServerApp\netImguiApp.rc");
			ResourceFiles.Add(@"[project.SharpmakeCsPath]\..\Code\ServerApp\Background.png");
		}

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.AddPublicDependency<ProjectImgui_Default>(target);
			conf.AddPublicDependency<ProjectNetImgui_Default>(target);
			conf.Options.Add(Options.Vc.Compiler.Exceptions.EnableWithExternC); // Because of std::Vector

			conf.IncludePaths.Add(SourceRootPath + @"\Source");
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\Code\ThirdParty\" + SolutionAll.sDefaultImguiVersion);
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\Code\Client");
			conf.EventPostBuild.Add(@"xcopy [project.SharpmakeCsPath]\..\Code\ServerApp\Background.png " + conf.TargetPath + " /D /Y");
		}
    }
	
	[Sharpmake.Generate]
    public class ProjectSample : ProjectBase
    {
        public ProjectSample(string inName)
		: base(true)
		{
            Name			= inName;
            SourceRootPath	= @"[project.SharpmakeCsPath]\..\Code\Sample\" + inName;
			AdditionalSourceRootPaths.Add(@"[project.SharpmakeCsPath]\..\Code\Sample\Common");
        }

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);			
			conf.AddPublicDependency<ProjectImgui_Default>(target);
			conf.AddPublicDependency<ProjectNetImgui_Default>(target);
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\Code\ThirdParty\" + SolutionAll.sDefaultImguiVersion);
			conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\Code\Client");
		}
    }
	
	[Sharpmake.Generate] public class ProjectSample_Basic 	: ProjectSample { public ProjectSample_Basic() 		: base("SampleBasic"){} }	
	[Sharpmake.Generate] public class ProjectSample_DualUI 	: ProjectSample { public ProjectSample_DualUI()		: base("SampleDualUI"){} }	
	[Sharpmake.Generate] public class ProjectSample_Textures: ProjectSample { public ProjectSample_Textures() 	: base("SampleTextures"){} }	
	[Sharpmake.Generate] public class ProjectSample_NewFrame: ProjectSample { public ProjectSample_NewFrame()	: base("SampleNewFrame"){} }	
	
	//=============================================================================================
	// SOLUTIONS
	//=============================================================================================
	public class SolutionBase : Sharpmake.Solution
    {
		public SolutionBase(string inName)
		: base(typeof(NetImguiTarget))
        {
			AddTargets(new NetImguiTarget { });
			Name					= inName;
			IsFileNameToLower		= false;
		}
 
        [Configure()]
        public void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			conf.Name				= "[target.Compiler]_[target.Optimization]";
            conf.SolutionFileName	= "[target.DevEnv]_[solution.Name]";
            conf.SolutionPath		= @"[solution.SharpmakeCsPath]\_Projects";
		}
	}
	
	[Sharpmake.Generate]
    public class SolutionSample : SolutionBase
    {
		public SolutionSample() : base("netImgui_Sample"){}

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {			
			base.ConfigureAll(conf, target);
			SolutionSample.AddProjects(conf, target);
		}
		
		public static void AddProjects(Configuration conf, NetImguiTarget target)
		{
			string SolutionFolder = @"Samples";
			conf.AddProject<ProjectSample_Basic>(target, false, SolutionFolder);			
			conf.AddProject<ProjectSample_NewFrame>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_DualUI>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_Textures>(target, false, SolutionFolder);
		}
	}

	[Sharpmake.Generate]
	public class SolutionServer : SolutionBase
	{
		public SolutionServer() : base("netImgui_Server") { }

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
		{
			base.ConfigureAll(conf, target);
			conf.AddProject<ProjectNetImguiServer>(target);
		}
	}

	[Sharpmake.Generate]
    public class SolutionAll : SolutionBase
    {
        public SolutionAll() : base("netImgui_All"){}

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.AddProject<ProjectNetImguiServer>(target);
			SolutionSample.AddProjects(conf, target);
			
			string SolutionFolder = @"Compatibility Tests";
			conf.AddProject<ProjectNetImgui_17500>(target, false, SolutionFolder);
			conf.AddProject<ProjectNetImgui_17600>(target, false, SolutionFolder);
			conf.AddProject<ProjectNetImgui_17700>(target, false, SolutionFolder);
			conf.AddProject<ProjectNetImgui_17800>(target, false, SolutionFolder);
			conf.AddProject<ProjectNetImgui_Dock17601>(target, false, SolutionFolder);
			conf.AddProject<ProjectNetImgui_Disabled>(target, false, SolutionFolder);			
		}
 
        [Sharpmake.Main]
        public static void SharpmakeMain(Sharpmake.Arguments arguments)
        {
            arguments.Generate<SolutionAll>();
			arguments.Generate<SolutionServer>();
			arguments.Generate<SolutionSample>();
        }
		
		static public string sDefaultImguiVersion	= @"Imgui_17800";
	}
}