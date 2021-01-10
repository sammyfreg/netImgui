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
	[Sharpmake.Generate] public class ProjectNetImgui_Default : ProjectNetImgui { public ProjectNetImgui_Default() : base(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath)) { Name = "NetImguiLib"; } }
	
	// Test compiling netImgui with the Disabled Define
	[Sharpmake.Generate] public class ProjectNetImgui_Disabled : ProjectNetImgui { 
		public ProjectNetImgui_Disabled() : base(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath)) { Name = "NetImguiLib (Disabled)"; }
		
		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
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
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_dx11.h");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_dx11.cpp");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_win32.h");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_win32.cpp");
			
			ResourceFiles.Add(NetImguiTarget.GetPath(@"\Code\ServerApp\small.ico"));
			ResourceFiles.Add(NetImguiTarget.GetPath(@"\Code\ServerApp\netImguiApp.ico"));
			ResourceFiles.Add(NetImguiTarget.GetPath(@"\Code\ServerApp\netImguiApp.rc"));
			ResourceFiles.Add(NetImguiTarget.GetPath(@"\Code\ServerApp\Background.png"));
			ResourceFiles.Add(NetImguiTarget.GetPath(@"\Code\ServerApp\Roboto-Medium.ttf"));
		}

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);
			conf.AddPublicDependency<ProjectImgui>(target);
			conf.AddPublicDependency<ProjectNetImgui_Default>(target);

			conf.IncludePaths.Add(SourceRootPath + @"\Source");
			conf.IncludePaths.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath));
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
			conf.EventPostBuild.Add(@"xcopy " + NetImguiTarget.GetPath(@"\Code\ServerApp\Background.png") + " " + conf.TargetPath + " /D /Y");
			conf.EventPostBuild.Add(@"xcopy " + NetImguiTarget.GetPath(@"\Code\ServerApp\Roboto-Medium.ttf") + " " + conf.TargetPath + " /D /Y");
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
			
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_dx11.h");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_dx11.cpp");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_win32.h");
			SourceFiles.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath) + @"\backends\imgui_impl_win32.cpp");
        }

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {
			base.ConfigureAll(conf, target);			
			conf.AddPublicDependency<ProjectImgui>(target);
			conf.AddPublicDependency<ProjectNetImgui_Disabled>(target);
			conf.IncludePaths.Add(NetImguiTarget.GetPath(ProjectImgui.sDefaultPath));
			conf.IncludePaths.Add(NetImguiTarget.GetPath(@"\Code\Client"));
			conf.Defines.Add("NETIMGUI_ENABLED=0");
		}
    }
	
	[Sharpmake.Generate] public class ProjectSample_Basic 	: ProjectSample { public ProjectSample_Basic() 		: base("SampleBasic"){} }	
	[Sharpmake.Generate] public class ProjectSample_DualUI 	: ProjectSample { public ProjectSample_DualUI()		: base("SampleDualUI"){} }	
	[Sharpmake.Generate] public class ProjectSample_Textures: ProjectSample { public ProjectSample_Textures() 	: base("SampleTextures"){} }	
	[Sharpmake.Generate] public class ProjectSample_NewFrame: ProjectSample { public ProjectSample_NewFrame()	: base("SampleNewFrame"){} }		
	
	//=============================================================================================
	// SOLUTIONS
	//=============================================================================================
	[Sharpmake.Generate]
    public class SolutionSample : SolutionBase
    {
		public SolutionSample() : base("netImgui_Sample"){}

		[Configure()]
		public new void ConfigureAll(Configuration conf, NetImguiTarget target)
        {			
			base.ConfigureAll(conf, target);
			SolutionSample.AddSampleProjects(conf, target);
		}
		
		public static void AddSampleProjects(Configuration conf, NetImguiTarget target)
		{
			string SolutionFolder = "Samples";
			conf.AddProject<ProjectSample_Basic>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_NewFrame>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_DualUI>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_Textures>(target, false, SolutionFolder);
			conf.AddProject<ProjectSample_Disabled>(target, false, SolutionFolder);
			
			// Moving an already auto added dependcy, so it can be moved to more appropriate folder
			conf.AddProject<ProjectNetImgui_Disabled>(target, false, "CompatibilityTest");
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