
#include "../ThirdParty/Imgui/imgui.h"
#include <RemoteImgui_CmdPackets.h>
#include <RemoteImgui_Client.h>
#include <RemoteImgui_Network.h>

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
#if 0
	// Update this to be platform independant
	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab]			= VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow]	= VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow]	= VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]		= VK_UP;
    io.KeyMap[ImGuiKey_DownArrow]	= VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp]		= VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown]	= VK_NEXT;
    io.KeyMap[ImGuiKey_Home]		= VK_HOME;
    io.KeyMap[ImGuiKey_End]			= VK_END;
    io.KeyMap[ImGuiKey_Delete]		= VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace]	= VK_BACK;
    io.KeyMap[ImGuiKey_Enter]		= VK_RETURN;
    io.KeyMap[ImGuiKey_Escape]		= VK_ESCAPE;
    io.KeyMap[ImGuiKey_A]			= 'A';
    io.KeyMap[ImGuiKey_C]			= 'C';
    io.KeyMap[ImGuiKey_V]			= 'V';
    io.KeyMap[ImGuiKey_X]			= 'X';
    io.KeyMap[ImGuiKey_Y]			= 'Y';
    io.KeyMap[ImGuiKey_Z]			= 'Z';
    io.ImeWindowHandle				= nullptr;//hWindow
#endif
	// Build
    unsigned char* pixels;
    int width, height;
	io.Fonts->AddFontDefault();
	io.Fonts->Build();
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	RmtImgui::Client::SendDataTexture(0, pixels, (uint16_t)width, (uint16_t)height, RmtImgui::kTexFmtRGBA);	   

    // Store our identifier
    io.Fonts->TexID = (void*)0;

    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();

	//if( !RmtImgui::Startup() )
	//	return false;
	uint8_t Ip[4] = {127,0,0,1};
	return RmtImgui::Client::Connect("ServerLogs", Ip, ServerPort);
}

void ServerInfoTab_Shutdown()
{

}

void ServerInfoTab_Draw()
{
	if( RmtImgui::Client::ReceiveDataInput(ImGui::GetIO()) )
	{
    //ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
//    RECT rect;
//    GetClientRect(ghMainWindow, &rect);
 //   io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
	
	//io.DisplaySize = ImVec2(gTempScreenSize[0],gTempScreenSize[1]);
    // Setup time step
    //INT64 current_time;
    //QueryPerformanceCounter((LARGE_INTEGER *)&current_time); 
    //io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
   // g_Time = current_time;

    // Read keyboard modifiers inputs
//    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
//    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
//    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    // io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
    // io.MousePos : filled by WM_MOUSEMOVE events
    // io.MouseDown : filled by WM_*BUTTON* events
    // io.MouseWheel : filled by WM_MOUSEWHEEL events

    // Hide OS mouse cursor if ImGui is drawing it
//    SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

		ImGui::NewFrame();
		ExampleAppLog::Get().Draw("Logs");	
		ImGui::Render();

		RmtImgui::Client::SendDataFrame( ImGui::GetDrawData() );
	}
	//RmtImgui::ImguiFrame* pNewFrame = RmtImgui::ImguiFrame::Create(ImGui::GetDrawData());
	//pNewFrame->UpdatePointer();
	// Replace pending Frame Data to be send. If there was another one still waiting,
	// take ownership of it and delete it
	//RmtImgui::ImguiFrame* pPendingDataPrev = nullptr; //gClients[0].mpFramePending.exchange(pNewFrame);	
	//if( pPendingDataPrev != 0 )
	//	free(pPendingDataPrev);

	//return;    
}

