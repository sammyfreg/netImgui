#include "stdafx.h"
#include <NetImgui_Api.h>
#include "ServerInfoTab.h"

//SF TODO: Implement proper logging and data transfer stats

//Class copied from Dear ImGui sample in "imgui_demo.cpp"
// Usage:
//  static ExampleAppLog my_log;
//  my_log.AddLog("Hello %d world\n", 123);
//  my_log.Draw("title");
struct ExampleAppLog
{
    ImGuiTextBuffer     Buf;
    ImGuiTextFilter     Filter;
    ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
    bool                AutoScroll;  // Keep scrolling if already at the bottom.
    uint8_t             PADDING[7];
    ExampleAppLog()
    {
        AutoScroll = true;
        Clear();
    }

    void    Clear()
    {
        Buf.clear();
        LineOffsets.clear();
        LineOffsets.push_back(0);
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        int old_size = Buf.size();
        va_list args;
        va_start(args, fmt);
        Buf.appendfv(fmt, args);
        va_end(args);
        for (int new_size = Buf.size(); old_size < new_size; old_size++)
            if (Buf[old_size] == '\n')
                LineOffsets.push_back(old_size + 1);
    }

    void    Draw(const char* title, bool* p_open = nullptr)
    {
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End();
            return;
        }

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &AutoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        Filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = Buf.begin();
        const char* buf_end = Buf.end();
        if (Filter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have a random access on the result on our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of
            // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
            for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
            {
                const char* line_start = buf + LineOffsets[line_no];
                const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                if (Filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            // The simplest and easy way to display the entire buffer:
            //   ImGui::TextUnformatted(buf_begin, buf_end);
            // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
            // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
            // within the visible area.
            // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
            // on your side is recommended. Using ImGuiListClipper requires
            // - A) random access into your data
            // - B) items all being the  same height,
            // both of which we can handle since we an array pointing to the beginning of each line of text.
            // When using the filter (in the block of code above) we don't have random access into the data to display
            // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
            // it possible (and would be recommended if you want to search through tens of thousands of entries).
            ImGuiListClipper clipper;
            clipper.Begin(LineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + LineOffsets[line_no];
                    const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }

	static ExampleAppLog& Get()
	{
        if( !spInstance )
            spInstance = new ExampleAppLog();
        
		return *spInstance;
	}
    
    static void Destroy()
    {
        if( spInstance )
        {
            delete spInstance;
            spInstance = nullptr;
        }
    }

protected:
    static ExampleAppLog* spInstance;    
};

ExampleAppLog* ExampleAppLog::spInstance = nullptr;

bool ServerInfoTab_Startup(unsigned int ServerPort)
{	
	if( !NetImgui::Startup() )
		return false;

	ImGui::SetCurrentContext(ImGui::CreateContext());
	ImGuiIO& io = ImGui::GetIO();	
	
	// Build
    unsigned char* pixels;
    int width, height;
	io.Fonts->AddFontDefault();
	io.Fonts->Build();
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	NetImgui::SendDataTexture(0, pixels, static_cast<uint16_t>(width), static_cast<uint16_t>(height), NetImgui::kTexFmtRGBA8);	   

    // Store our identifier
    io.Fonts->TexID = reinterpret_cast<ImTextureID*>(0);

    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

	if( !NetImgui::Connect("ServerLogs", "localhost", ServerPort, false) )
		return false;

	return true;
}

void ServerInfoTab_Shutdown()
{
    ExampleAppLog::Destroy();
	ImGui::DestroyContext(ImGui::GetCurrentContext());
}

void ServerInfoTab_Draw(uint32_t clientCount)
{
	if( NetImgui::NewFrame() )
	{        		
        ImGui::BeginMainMenuBar();
        ImGui::Text(clientCount <= 1 ? "Waiting for Connection..." : "Client connected : %i", clientCount - 1);
        ImGui::EndMainMenuBar();        
		NetImgui::EndFrame();
        //ExampleAppLog::Get().Draw("Logs");
	}
}

