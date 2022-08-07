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
		MSBuild = 1 << 1,
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
			Platform = Platform.win64 | Platform.win32;
			Optimization = Optimization.Debug | Optimization.Release;
			Compiler = Compiler.MSBuild | Compiler.Clang;
		}
		
		// Generates a solution for each Visual Studio version found
		// Note: Add a Clang target when detected installed for that Visual Studio version
		static public NetImguiTarget[] CreateTargets()
		{		
			List<NetImguiTarget> targets = new List<NetImguiTarget>();
			foreach (var devEnv in new [] { DevEnv.vs2017, DevEnv.vs2019, DevEnv.vs2022 })
			{				
				if( Util.DirectoryExists(devEnv.GetVisualStudioDir()) ){
					Compiler compiler = Compiler.MSBuild;
					if(Util.FileExists(ClangForWindows.Settings.LLVMInstallDirVsEmbedded(devEnv) + @"\bin\clang.exe" )){
						compiler |= Compiler.Clang;
					}
					targets.Add(new NetImguiTarget{DevEnv = devEnv, Compiler = compiler});
				}
			}
			return targets.ToArray();
		}
		
		static public string GetPath(string pathFromRoot)
		{
			return Directory.GetCurrentDirectory() + pathFromRoot;
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
			CustomBuildFileHLSL.AddFilesExt(this);
			IsFileNameToLower		= false;
			IsTargetFileNameToLower = false;
			mIsExe					= isExe;
			AddTargets(NetImguiTarget.CreateTargets());
		}

		protected override void ExcludeOutputFiles()
		{
			base.ExcludeOutputFiles();
			CustomBuildFileHLSL.ClaimAllShaderFiles(this);			
		}
		
		[Configure()]
        public virtual void ConfigureAll(Configuration conf, NetImguiTarget target)
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
			
			if ( target.Compiler == Compiler.Clang ){
				conf.Options.Add(Options.Vc.General.PlatformToolset.ClangCL);
				conf.AdditionalCompilerOptions.Add("-Wno-unused-command-line-argument"); //Note: Latest Clang doesn't support '/MP' (multiprocessor build) option, creating a compile error
			}
			if (mIsExe)
			{
				conf.VcxprojUserFile = new ProjConfig.VcxprojUserFileSettings();
				conf.VcxprojUserFile.LocalDebuggerWorkingDirectory = "$(TargetDir)";
				conf.Options.Add(Options.Vc.Linker.SubSystem.Windows);
				conf.LibraryFiles.Add("D3D11.lib");
				conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code"));
			}

			conf.Options.Add(Options.Vc.General.WindowsTargetPlatformVersion.Latest);
			conf.Options.Add(Options.Vc.General.TreatWarningsAsErrors.Enable);
			conf.Options.Add(Options.Vc.General.CharacterSet.Unicode);			
			conf.Options.Add(Options.Vc.Linker.TreatLinkerWarningAsErrors.Enable);
			
			conf.Defines.Add("_HAS_EXCEPTIONS=0"); 					// Prevents error in VisualStudio c++ library with NoExcept, like xlocale
			conf.Defines.Add("IMGUI_DISABLE_OBSOLETE_FUNCTIONS");	// Enforce using up to date Dear ImGui Api (In Server, Compatibility tests and Samples)
			
			if (target.Optimization == Optimization.Debug){
				conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);				
				// Note: Once Clang debug library link error is fixed (in new clang release),
				// try removing 'MultiThreadedDebugDLL' and enabling asan for clang too
				if( target.DevEnv > DevEnv.vs2017 && target.Compiler == Compiler.MSBuild ){
					conf.Options.Add(Options.Vc.Compiler.EnableAsan.Enable);
				}
			}
            else{
                conf.Options.Add(Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);
			}
			
			if( target.Compiler == Compiler.MSBuild ){
				conf.Options.Add(new Options.Vc.Compiler.DisableSpecificWarnings(""));
				conf.Options.Add(Options.Vc.Librarian.TreatLibWarningAsErrors.Enable);	//Note: Clang VS2019 doesn't support this option properly
			}
		}
		
		// Add sources files for platform specific Dear ImGui backend (OS/Renderer support)
		public void AddImguiBackendSources()
		{
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_dx11.h");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_dx11.cpp");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_win32.h");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_win32.cpp");
		}
		
		public void AddDependencyImguiIndex16(Configuration conf, NetImguiTarget target)
		{
			conf.AddPublicDependency<ProjectImguiIndex16>(target);
			EnabledImguiIndex16Bits(conf);
		}
		
		public void AddDependencyImguiIndex32(Configuration conf, NetImguiTarget target)
		{
			conf.AddPublicDependency<ProjectImguiIndex32>(target);			
			EnabledImguiIndex32Bits(conf);
		}
	
		public void AddDependencyImguiServer(Configuration conf, NetImguiTarget target)
		{
			conf.AddPublicDependency<ProjectImguiServer>(target);			
			EnabledImguiIndex32Bits(conf);
		}
		
		public void EnabledImguiIndex16Bits(Configuration conf)
		{
		}
		
		public void EnabledImguiIndex32Bits(Configuration conf)
		{
			conf.Defines.Add("ImDrawIdx=unsigned int");
		}
		
		bool mIsExe;
	}

	//---------------------------------------------------------------------------------------------
	// IMGUI Project
	//---------------------------------------------------------------------------------------------
	[Sharpmake.Generate]
	public class ProjectImgui : ProjectBase
	{
		public ProjectImgui()
		: base(false)
		{			
			SourceRootPath = NetImguiTarget.GetPath(sDefaultPath);
			SourceFilesExcludeRegex.Add(@"backends\.*");
		}
		
		public static string sDefaultPath = @"\Code\ThirdParty\DearImgui";
	}
	
	// Dear ImGui Library, 16bits index
	[Sharpmake.Generate]
	public class ProjectImguiIndex16 : ProjectImgui
	{
		public ProjectImguiIndex16() { Name = "DearImguiIndex16Lib"; }
		
		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			EnabledImguiIndex16Bits(conf);
		}
	}

	// Dear ImGui Library, 32bits index
	[Sharpmake.Generate] 
	public class ProjectImguiIndex32 : ProjectImgui 
	{ 
		public ProjectImguiIndex32() { Name = "DearImguiIndex32Lib"; }
		
		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			EnabledImguiIndex32Bits(conf);
		}
	}
	
	// Dear ImGui Library, 32bits index & 64 bits textureID
	[Sharpmake.Generate] 
	public class ProjectImguiServer : ProjectImgui 
	{ 
		public ProjectImguiServer() { Name = "DearImguiServerLib"; }
		
		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.Defines.Add("ImTextureID=ImU64");		// Server must absolutly use at minimum 64bits texture id, even when compiled in 32 bits			
			EnabledImguiIndex32Bits(conf);
		}
	}
	
	//---------------------------------------------------------------------------------------------
	// NETIMGUI Project
	//---------------------------------------------------------------------------------------------
	[Sharpmake.Generate]
    public class ProjectNetImgui : ProjectBase
    {
		public ProjectNetImgui(string imguiFullPath)
		: base(false)
        {
			mVersion 		= Path.GetFileName(imguiFullPath);
			mImguiPath 		= imguiFullPath;
			Name 			= "NetImguiLib (" + mVersion + ")";
            SourceRootPath 	= NetImguiTarget.GetPath(@"\Code\Client");
			SourceFiles.Add(mImguiPath + @"\imgui.h");
        }

		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
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
        public ProjectSample(string inName, bool useIndex32=false)
		: base(true)
		{
            Name			= inName;
			mUseIndex32		= useIndex32;
            SourceRootPath	= NetImguiTarget.GetPath(@"\Code\Sample\") + Name;
			AdditionalSourceRootPaths.Add(NetImguiTarget.GetPath(@"\Code\Sample\Common"));
			AddImguiBackendSources();			
        }

		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			if( mUseIndex32 == false ){
				AddDependencyImguiIndex16(conf, target);
				conf.AddPublicDependency<ProjectNetImgui16_Default>(target);
			}
			else
			{
				AddDependencyImguiIndex32(conf, target);
				conf.AddPublicDependency<ProjectNetImgui32_Default>(target);
			}
			conf.IncludePaths.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath));
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
		}
		bool mUseIndex32;
    }
	
	// Compile a console program, with Dear ImGui and NetImgui sources 
	// included directly. The Dear ImGui code does not include any backend,
	// only try connecting the the NetImgui Server to draw its content remotely.
	[Sharpmake.Generate] 
	public class ProjectNoBackend : ProjectBase 
	{
		public ProjectNoBackend(string inName, string inImguiFullPath)
		: base(true)
		{
			mImguiFullPath	= string.IsNullOrEmpty(inImguiFullPath) ? NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) : inImguiFullPath;
			Name			= inName;
            SourceRootPath	= NetImguiTarget.GetPath(@"\Code\Sample\SampleNoBackend");
			
			// Find the Dear Imgui Sources files
			string[] sourceExtensions = new string[]{".h",".cpp"};
			var files = Directory.EnumerateFiles(mImguiFullPath, "*.*", SearchOption.TopDirectoryOnly);
			foreach (var file in files)
			{				
				if (sourceExtensions.Contains(Path.GetExtension(file), StringComparer.OrdinalIgnoreCase)){
					//Console.WriteLine("File Added: {0}", Path.GetFullPath(file));
					SourceFiles.Add(Path.GetFullPath(file));
				}
			};
		}
		
		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
		{
			base.ConfigureAll(conf, target);
			conf.IncludePaths.Add(mImguiFullPath);
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
			conf.Options.Add(Options.Vc.Linker.SubSystem.Console);
			conf.LibraryFiles.Add("ws2_32.lib");
			
			// Remove a some Dear ImGui sources compile warning
			if( target.Compiler == Compiler.MSBuild ){
				conf.Options.Add(new Options.Vc.Compiler.DisableSpecificWarnings("4100")); // warning C4100: xxx: unreferenced formal parameter
				conf.Options.Add(new Options.Vc.Compiler.DisableSpecificWarnings("4189")); // warning C4189: xxx: unused local variable
			}
			else if ( target.Compiler == Compiler.Clang ){
				conf.Options.Add(Options.Vc.General.PlatformToolset.ClangCL);				
				conf.AdditionalCompilerOptions.Add("-Wno-unknown-warning-option");
				conf.AdditionalCompilerOptions.Add("-Wno-unused-parameter");
				conf.AdditionalCompilerOptions.Add("-Wno-unused-variable");
				conf.AdditionalCompilerOptions.Add("-Wno-unused-but-set-variable");
			}
		}
		string mImguiFullPath;
	}
	
	//=============================================================================================
	// SOLUTIONS
	//=============================================================================================
	public class SolutionBase : Sharpmake.Solution
    {
		public SolutionBase(string inName)
		: base(typeof(NetImguiTarget))
        {
			Name					= inName;
			IsFileNameToLower		= false;			
			AddTargets(NetImguiTarget.CreateTargets());
		}
 
        [Configure()]
        public virtual void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			conf.Name				= "[target.Compiler]_[target.Optimization]";
            conf.SolutionFileName	= "[target.DevEnv]_[solution.Name]";
            conf.SolutionPath		= NetImguiTarget.GetPath(@"\_projects");
		}
	}
}