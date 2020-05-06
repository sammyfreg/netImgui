#include <chrono>
#include <NetImGui_Api.h>
#include "../ThirdParty/Imgui/imgui.h"

//Class copied from Dear Imgui sample in "imgui_demo.cpp"
struct ExampleAppLog
{
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets;        // Index to lines offset
    bool                ScrollToBottom;

    void    Clear()     { Buf.clear(); LineOffsets.clear(); }

    void    AddLog(const char* fmt, ...) IM_PRINTFARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size);
        ScrollToBottom = true;
    }

    void    Draw(const char* title, bool* p_opened = nullptr)
    {
        ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiSetCond_FirstUseEver);
        ImGui::Begin(title, p_opened);
        if (ImGui::Button("Clear")) Clear();
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);
        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (copy) ImGui::LogToClipboard();

        if (Filter.IsActive())
        {
            const char* buf_begin = Buf.begin();
            const char* line = buf_begin;
            for (int line_no = 0; line != nullptr; line_no++)
            {
                const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : nullptr;
                if (Filter.PassFilter(line, line_end))
                    ImGui::TextUnformatted(line, line_end);
                line = line_end && line_end[1] ? line_end + 1 : nullptr;
            }
        }
        else
        {
            ImGui::TextUnformatted(Buf.begin());
        }

        if (ScrollToBottom)
            ImGui::SetScrollHere(1.0f);
        ScrollToBottom = false;
        ImGui::EndChild();
        ImGui::End();
    }

	static ExampleAppLog& Get()
	{
		static ExampleAppLog ImguiLog;
		return ImguiLog;
	}
};

bool ServerInfoTab_Startup(unsigned int ServerPort)
{
	ImGuiIO& io = ImGui::GetIO();
	if( !NetImgui::Startup() )
		return false;

	uint8_t Ip[4] = {127,0,0,1};
	if( !NetImgui::Connect(io, "ServerLogs", Ip, ServerPort) )
		return false;

	// Build
    unsigned char* pixels;
    int width, height;
	io.Fonts->AddFontDefault();
	io.Fonts->Build();
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	NetImgui::SendDataTexture(0, pixels, (uint16_t)width, (uint16_t)height, NetImgui::kTexFmtRGBA8);	   

    // Store our identifier
    io.Fonts->TexID = (void*)0;

    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

	return true;
}

void ServerInfoTab_Shutdown()
{

}

void ServerInfoTab_Draw()
{
	if( NetImgui::InputUpdateData() )
	{
		static auto lastTime			= std::chrono::high_resolution_clock::now();
		auto currentTime				= std::chrono::high_resolution_clock::now();
		auto duration					= std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);
		ImGui::GetIO().DeltaTime		= static_cast<float>(duration.count() / 1000.f);
		lastTime						= currentTime;

		ImGui::NewFrame();
		ExampleAppLog::Get().Draw("Logs");	
		ImGui::Render();

		NetImgui::SendDataDraw( ImGui::GetDrawData() );
	}
}

