//=================================================================================================
// @SAMPLE_EDIT
// Note:    This file a duplicate of 'Dear ImGui Sample' : "examples\example_glfw_opengl3\main.cpp"
//          With a few editions added to customize it for our NetImGui server needs. 
//          These fews edits will be found in a few location, using the tag '@SAMPLE_EDIT' 
#include "NetImguiServer_App.h"

#if HAL_API_PLATFORM_SOKOL

#include <string>
#include <iostream>
#include "NetImguiServer_UI.h"

#define SOKOL_IMPL
#define SOKOL_NO_ENTRY
#ifdef __EMSCRIPTEN__
#define SOKOL_GLES3
#else
#define SOKOL_GLCORE33
#endif
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_log.h"
#include "imgui.h"
#include "sokol/util/sokol_imgui.h"

sg_pass_action pass_action{};
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
std::string cmdArgs;

void init() {
    sg_desc desc = {};
    sg_context_desc context_desc = sapp_sgcontext();
    context_desc.depth_format = SG_PIXELFORMAT_NONE;
    desc.context = context_desc;
    desc.logger.func = slog_func;
    desc.disable_validation = true;
    sg_setup(&desc);

    simgui_desc_t simgui_desc = {};
    simgui_desc.no_default_font = true;
    simgui_desc.depth_format = SG_PIXELFORMAT_NONE;
    simgui_setup(&simgui_desc);

    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    bool ok = NetImguiServer::App::Startup(cmdArgs.c_str());
    IM_ASSERT(ok);
    (VOID)ok;

    // IMGUI Font texture init
    {
        // Build texture atlas
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

        sg_image_desc img_desc{};
        img_desc.width = width;
        img_desc.height = height;
        img_desc.label = "font-texture";
        img_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
        img_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
        img_desc.min_filter = SG_FILTER_LINEAR;
        img_desc.mag_filter = SG_FILTER_LINEAR;
        img_desc.data.subimage[0][0].ptr = pixels;
        img_desc.data.subimage[0][0].size = (width * height) * sizeof(uint32_t);

        sg_image img = sg_make_image(&img_desc);

        io.Fonts->TexID = (ImTextureID)(uintptr_t)img.id;
    }
}

void frame() {
    const int width = sapp_width();
    const int height = sapp_height();

//    sg_begin_default_pass(&pass_action, width, height);

    simgui_new_frame({ width, height, sapp_frame_duration(), sapp_dpi_scale() });
    NetImguiServer::App::UpdateRemoteContent(); // @SAMPLE_EDIT (Request each client to update their drawing content )
    clear_color = NetImguiServer::UI::DrawImguiContent();
    pass_action.colors[0].clear_value.r = clear_color.x;
    pass_action.colors[0].clear_value.g = clear_color.y;
    pass_action.colors[0].clear_value.b = clear_color.z;
    pass_action.colors[0].clear_value.a = clear_color.w;

    sg_begin_default_pass(&pass_action, width, height);
    simgui_render();
    sg_end_pass();

    sg_commit();
}

void cleanup() {
    simgui_shutdown();
    sg_shutdown();
}

void input(const sapp_event* event) {
    simgui_handle_event(event);
}

int main(int argc, const char* argv[]) {
    for (size_t i = 0; i < size_t(argc); ++i) {
        std::string arg(argv[i]);
        cmdArgs += arg + " ";
    }

    if (!cmdArgs.empty()) {
        cmdArgs.pop_back();
    }


    sapp_desc desc = {};
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup,
    desc.event_cb = input,
    desc.width = 1280,
    desc.height = 720,
    desc.window_title = "NetImgui Server",
    desc.icon.sokol_default = true,
    desc.logger.func = slog_func;
    sapp_run(desc);

    return EXIT_SUCCESS;
}

#endif // @SAMPLE_EDIT HAL_API_PLATFORM_SOKOL
