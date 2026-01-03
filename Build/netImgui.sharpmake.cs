// NetImgui.sharpmake.cs
using Sharpmake;
using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;
using System.IO;
using System.Text.RegularExpressions;

namespace NetImgui
{	
	//=============================================================================================
	// PROJECTS
	//=============================================================================================
	// Generate the default Imgui/netImgui Libraries (used to link against with Samples/Server)
	[Sharpmake.Generate] public class ProjectNetImgui16_Default : ProjectNetImgui
	{ 
		public ProjectNetImgui16_Default() : base(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath)) { Name = "NetImgui16Lib"; } 

		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
            conf.Defines.Add("NETIMGUI_ENABLED=1");
        }
	}
	
	[Sharpmake.Generate] public class ProjectNetImgui32_Default : ProjectNetImgui
	{ 
		public ProjectNetImgui32_Default() : base(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath)) { Name = "NetImgui32Lib"; } 
		
		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.Defines.Add("ImDrawIdx=unsigned int");
            conf.Defines.Add("NETIMGUI_ENABLED=1");
        }
	}
	
	// Test compiling netImgui with the Disabled Define
	[Sharpmake.Generate] public class ProjectNetImgui_Disabled : ProjectNetImgui
	{ 
		public ProjectNetImgui_Disabled() : base(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath)) { Name = "NetImguiLib (Disabled)"; }
		
		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.Defines.Add("NETIMGUI_ENABLED=0");
		}
	}
	
	[Sharpmake.Generate]
    public class ProjectNetImguiServer : ProjectBase
    {
        public ProjectNetImguiServer()
		: base(true)
		{
            Name			= "NetImguiServer"; 
            SourceRootPath	= NetImguiTarget.GetPath(@"\Code\ServerApp");
			ResourceFiles.Add(NetImguiTarget.GetPath(@"\Code\ServerApp\small.ico"));
			ResourceFiles.Add(NetImguiTarget.GetPath(@"\Code\ServerApp\netImguiApp.ico"));
			ResourceFiles.Add(NetImguiTarget.GetPath(@"\Code\ServerApp\netImguiApp.rc"));
			ResourceFiles.Add(NetImguiTarget.GetPath(@"\Code\ServerApp\Background.png"));
			SourceFilesBuildExcludeRegex.Add(@".*Code\\ServerApp\\Source\\Fonts\\.*");
			
			// Want the Dear Imgui Backend source files listed in the project, but not compiled
			// (we selectively include them in the build, based on the HAL)
			AdditionalSourceRootPaths.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends");
			SourceFilesBuildExcludeRegex.Add(@"backends\\");
			
			//---------------------------------------------
			// For the OpenGL Server build
			AdditionalSourceRootPaths.Add(NetImguiTarget.GetPath(@"\Code\ThirdParty\glfw\include"));
			AdditionalSourceRootPaths.Add(NetImguiTarget.GetPath(@"\Code\ThirdParty\glad30core\include"));
			AdditionalSourceRootPaths.Add(NetImguiTarget.GetPath(@"\Code\ThirdParty\glad30core\src"));
			SourceFilesBuildExcludeRegex.Add(@"ThirdParty\\glfw\\");
			//---------------------------------------------
		}

		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			
			AddDependencyImguiServer(conf, target);
			conf.AddPublicDependency<ProjectNetImgui32_Default>(target);
			
			conf.Defines.Add("IS_NETIMGUISERVER=1");	// 
			conf.Defines.Add("ImTextureUserID=ImU64");		// Server must absolutly use at minimum 64bits texture id, even when compiled in 32 bits			
			
			conf.IncludePaths.Add(SourceRootPath + @"\Source");
			conf.IncludePaths.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath));
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
			
			//---------------------------------------------
			// For the OpenGL Server build
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\ThirdParty\glfw\include"));
            conf.LibraryPaths.Add(NetImguiTarget.GetPath(@"\Code\ThirdParty\glfw\" + getGlfwLibName(target.Platform, target.DevEnv)));
			conf.Options.Add(new Options.Vc.Linker.DisableSpecificWarnings("4099")); //Prevents: warning LNK4099: PDB '' was not found with 'glfw3_mtd.lib(context.c.obj)' or at ''; linking object as if no debug info
			//---------------------------------------------
			
			if (target.DevEnv == DevEnv.xcode)
			{
				conf.Options.Add(new Sharpmake.Options.XCode.Compiler.InfoPListFile(NetImguiTarget.GetPath(@"/Code/ServerApp/info.plist")));
			}
			else
			{
				conf.EventPostBuild.Add("xcopy \"" + NetImguiTarget.GetPath(@"\Code\ServerApp\Background.png") + "\" \"" + conf.TargetPath + "\" /D /Y");
			}
			
			if ( target.Compiler == Compiler.Clang ){
				conf.AdditionalCompilerOptions.Add("-Wno-deprecated-literal-operator"); // caused by json header include
			}
		}

		private string getGlfwLibName(Platform platform, DevEnv developerEnv)
		{
			string libName = "lib";
			if( developerEnv == DevEnv.vs2026 ){
			   libName += "-vc2026";
			} else if( developerEnv == DevEnv.vs2022 ){
			   libName += "-vc2022";
			} else if( developerEnv == DevEnv.vs2019 ) {
                libName += "-vc2019";
			} else if( developerEnv == DevEnv.vs2017 ) {
                libName += "-vc2017";
			} else if( developerEnv == DevEnv.xcode ) {
                libName += "-macos-universal";
            }

            if (platform == Platform.win64) {
                libName += "-64";
            } else if (platform == Platform.win32) {
                libName += "-32";
            }

            return libName;
		}
    }
	
	[Sharpmake.Generate]
    public class ProjectSample_Disabled : ProjectBase
    {
        public ProjectSample_Disabled()
		: base(true)
		{
            Name			= "SampleDisabled";
            SourceRootPath	= NetImguiTarget.GetPath(@"\Code\Sample\") + Name;
			AdditionalSourceRootPaths.Add(NetImguiTarget.GetPath(@"\Code\Sample\Common"));			
			AddImguiBackendSources();
        }

		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			AddDependencyImguiIndex16(conf, target);
			conf.AddPublicDependency<ProjectNetImgui_Disabled>(target);
			conf.IncludePaths.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath));
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
			conf.Defines.Add("NETIMGUI_ENABLED=0");
		}
    }
	//-------------------------------------------------------------------------
	// Standard samples
	//-------------------------------------------------------------------------
	[Sharpmake.Generate] public class ProjectSample_Basic 		: ProjectSample { public ProjectSample_Basic() 		: base("SampleBasic"){} }
	[Sharpmake.Generate] public class ProjectSample_Textures	: ProjectSample { public ProjectSample_Textures() 	: base("SampleTextures"){} }
	[Sharpmake.Generate] public class ProjectSample_NewFrame	: ProjectSample { public ProjectSample_NewFrame()	: base("SampleNewFrame"){} }
	[Sharpmake.Generate] public class ProjectSample_Background	: ProjectSample { public ProjectSample_Background()	: base("SampleBackground"){} }
	[Sharpmake.Generate] public class ProjectSample_Index16Bits	: ProjectSample { public ProjectSample_Index16Bits(): base("SampleIndex"){ Name = "SampleIndex16Bits"; } }
	[Sharpmake.Generate] public class ProjectSample_Index32Bits	: ProjectSample { public ProjectSample_Index32Bits(): base("SampleIndex", true){ Name = "SampleIndex32Bits"; } }	

	//-------------------------------------------------------------------------
	// Sample with more config overrides
	//-------------------------------------------------------------------------
	[Sharpmake.Generate]
    public class ProjectSample_SingleInclude : ProjectBase
    {
		// This sample does not includes the NetImgui Library.
		// Instead it includes and compiled the NetImgui sources files 
		// directly, alongside it own sources files. This is to demonstrate
		// being able to compile the NetImgui support with only 1 include.
        public ProjectSample_SingleInclude()
		: base(true)
		{
            Name			= "SampleSingleInclude";
            SourceRootPath	= NetImguiTarget.GetPath(@"\Code\Sample\") + Name;
			AdditionalSourceRootPaths.Add(NetImguiTarget.GetPath(@"\Code\Sample\Common"));			
			AddImguiBackendSources();
        }

		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);			
			AddDependencyImguiIndex16(conf, target);
			conf.IncludePaths.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath));
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
		}
    }
	
	//-------------------------------------------------------------------------
	// This sample does not have a UI Window, only show a console and wait 
	// for a connection to NetImguiServer. when connected, Display its
	// Dear ImGui content normally, on the remote server. 
	// Usefull to demonstrate being able to use NetImgui without even needing to 
	// implement a Backend support (windows / renderer / input) on the client.
	// It also compiles the Dear ImGui/NetImgui sources directly
	//-------------------------------------------------------------------------
	[Sharpmake.Generate]
    public class ProjectSample_NoBackend : ProjectNoBackend
    {
        public ProjectSample_NoBackend()
		: base("SampleNoBackend","")
		{
        }
    }
	
	//
	[Sharpmake.Generate] 
	public class ProjectSample_Compression : ProjectBase 
	{
		// This sample does not includes the NetImgui Library.
		// Instead it includes and compiled the NetImgui sources files 
		// directly, alongside it own sources files. This is to ignore
		// the communications source file included by default and use
		// its own custom version instead (SampleNetworkWin32.cpp) to add
		// some metrics measure
		public ProjectSample_Compression()
		: base(true)
		{
			Name			= "SampleCompression";
            SourceRootPath	= NetImguiTarget.GetPath(@"\Code\Sample\") + Name;
			AdditionalSourceRootPaths.Add(NetImguiTarget.GetPath(@"\Code\Sample\Common"));			
			AddImguiBackendSources();
		}
		
		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
		{
			base.ConfigureAll(conf, target);
			AddDependencyImguiIndex16(conf, target);
			conf.IncludePaths.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath));
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
			// Disable the built-in communication library to implement a custom one
			conf.Defines.Add("NETIMGUI_WINSOCKET_ENABLED=0");
			conf.Defines.Add("NETIMGUI_POSIX_SOCKETS_ENABLED=0");
		}
	}
	
	//
	[Sharpmake.Generate] 
	public class ProjectSample_SampleCompatibility : ProjectBase 
	{
		// This sample does not includes the Dear Imgui or NetImgui Library included.
		// They are instead compiled inside this project. This allows to test various
		// older version of Dear ImGui against our NetImgui Server compiled 
		// with latest version
		public ProjectSample_SampleCompatibility()
		: base(true)
		{
			Name			= "Compatibility";
            SourceRootPath	= NetImguiTarget.GetPath(@"\Code\Sample\SampleCompatibility");
			SourceFiles.Add(@"C:\GitHub\NetImguiDev\Build\..\_generated\imgui\imgui-1.88\*.cpp");
			SourceFiles.Add(@"C:\GitHub\NetImguiDev\Build\..\_generated\imgui\imgui-1.88\*.h");
		}
		
		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
		{
			base.ConfigureAll(conf, target);
			conf.IncludePaths.Add(@"C:\GitHub\NetImguiDev\Build\..\_generated\imgui\imgui-1.88\");
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
			
			conf.Options.Add(Options.Vc.Linker.SubSystem.Console); 
			
		}
	}
	
	//=============================================================================================
	// SOLUTIONS
	//=============================================================================================
	[Sharpmake.Generate]
    public class SolutionSample : SolutionBase
    {
		public SolutionSample() : base("netImgui_Sample"){}

		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {			
			base.ConfigureAll(conf, target);
			SolutionSample.AddSampleProjects(conf, target);
		}
		
		public static void AddSampleProjects(Configuration conf, NetImguiTarget target)
		{
			string SolutionFolder = "Samples";
			conf.AddProject<ProjectSample_Basic>(target, false, SolutionFolder);			
			conf.AddProject<ProjectSample_NewFrame>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_Compression>(target, false, SolutionFolder);			
			conf.AddProject<ProjectSample_Textures>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_Background>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_Index16Bits>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_Index32Bits>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_Disabled>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_SingleInclude>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_NoBackend>(target, false, SolutionFolder);
			
			// Adding an already auto included dependency, so it can be moved to more appropriate folder
			conf.AddProject<ProjectNetImgui_Disabled>(target, false, "CompatibilityTest");
		}
	}

	[Sharpmake.Generate]
	public class SolutionServer : SolutionBase
	{
		public SolutionServer() : base("netImgui_Server") { }

		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
		{
			base.ConfigureAll(conf, target);
			conf.AddProject<ProjectNetImguiServer>(target);
		}
	}

	[Sharpmake.Generate]
    public class SolutionAll : SolutionBase
    {
        public SolutionAll() : base("netImgui_All"){}
		
		public override void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.AddProject<ProjectNetImguiServer>(target);
			SolutionSample.AddSampleProjects(conf, target);
			Utility.AddCompatibilityProjects(conf, target);
		}
 
        [Sharpmake.Main]
        public static void SharpmakeMain(Sharpmake.Arguments arguments)
        {
            arguments.Generate<SolutionAll>();
			arguments.Generate<SolutionServer>();
			arguments.Generate<SolutionSample>();
        }		
	}
}
