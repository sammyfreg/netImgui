 <p align="center"><img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImguiLogo.png" width=128 height=128></p>

# Summary
**NetImgui** is a library to remotely display and control **[Dear ImGui](https://github.com/ocornut/imgui)** menus with an associated **netImgui server** application. Designed to painlessly integrate into existing codebase with few changes required to the existing logic. It forwards the rendering information needed to draw the UI on another PC (vertices, indices, draw commands).

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImgui.png)

# Purpose
Initially created to assist game developers in debugging their game from a PC while running on console or cellphone. However, its use can easily be extended to other fields. For example, help with VR developement, or easily add a UI to some devices without display but TCP/IP communications available.

#### 1. Input ease of use
For programs running on hardware without easy access to a keyboard and mouse, **netImgui** provides the  comfort of your computer's inputs while controlling it remotely.

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/InputWithNetImgui.gif)

#### 2. Declutter display
**Dear ImGui** is often used to display relevant debug information during development, but its UI elements can obscure the regular content. **NetImgui** sends the UI content to a remote window, leaving the original display clutter-free and with the freedom to use the entire screen for more elaborate content. The hardware running **Dear ImGui** might not even have a display, sending the UI content to the **NetImgui** server application allows to have one.

###### Before
![DearImGui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/AppWithoutNetImgui.png)

###### With netImgui
![netImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/AppWithNetImguiGif.gif)

# Integration
- Download the latest version of **netImgui**.
- Add the content of ***Code\Client*** to your codebase.
- In your codebase :
  - [once] Call `NetImgui::ConnectToApp()` or `NetImgui::ConnectFromApp()`.
  - [once] Call `NetImgui::SendDataTexture()` *(uploads your font texture to netImguiServer)*.
  - [Every Redraw]
    - Replace `ImGui::NewFrame()` with call to `NetImgui::NewFrame()`.
    - Draw your ImGui menu as usual.
    - Replace `ImGui::Render()` and `ImGui::EndFrame()` with call to `NetImgui::EndFrame()`.
- Start the **netImgui** server application and connect your application to it

(More integration details can be found on the [Wiki](https://github.com/sammyfreg/netImgui/wiki "Wiki") and multiple included Sample projects can provide insights)

#### Note
- Connection between **netImgui Server** and a **netImGui Client** can be achieved in 4 different ways.
 - **Server waits for connection and** :
   - (A) Client calls `ConnectToApp()` with the Server address.
 - **Client calls `ConnectFromApp()` then waits for connection and** :
   - (B) Server configure the Client's address and connect to it.
   - (C) Server is launched with the Client's address in commandline.
   - (D) Server receives a Client's address from another application, through [Windows named pipe](https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipes "Windows named pipe") : \\.\pipe\netImgui'.
 
 
- **Advanced:** Different **Dear ImGui** content can be displayed locally and remotely at the same time. Look at *SampleDualUI* for more information.

- `NetImgui::IsConnected()` and `NetImgui::IsDrawingRemote()` can be used during Dear ImGui drawing, helping to make selective decisions on the content to draw based on where it will be displayed.

#### Dear Imgui versions
Tested against these **Dear ImGui** versions: **1.74**, **1.75**, **1.76**, **1.76**(Docking), **1.77**, **1.78**

Should be able to support other versions without too much difficulty. Mostly involve making sure the function `ContextClone()` compiles properly against the `imgui.h` file.

# Release notes
### To do
- Bugfix : netImgui Server application unable to start when post configured for listening can't be opened. Need to manually edit `netImgui.cfg` with a valid port.
- Support of additional texture formats
- Commands to assign custom backgrounds
- Handle Linear/sRGB vertex color format
- Add logging information in netImgui server application
- Profile and optimize performances
- Add copy/paste support
- Add new **Dear ImGui** multi windows support (docking branch)
- ~~Networking: Add support of client accepting connection from netImgui App~~

### Version 1.2
(2020/08/22)
- **API Changes**
 - `NetImGui::NewFrame()` / `NetImGui::EndFrame()` should always be used, even when disconnected
 - `NetImGui::NewFrame()` takes a new parameter, telling **netImgui** to continue using the same context or use a duplicate
 - `NetImgui::Connect()` replaced by `NetImgui::ConnectToApp()` / `NetImGui::ConnectFromApp()`
 - `NetImgui::IsRemoteDraw()` renamed to `NetImgui::IsDrawingRemote()`
 - `NetImgui::IsDrawing()` added
 - `NetImgui::IsConnectionPending()` added
 - `NetImgui::GetDrawData()` added
 - `NetImgui::GetRemoteContext()` removed
- **New** 
 - Support for connection initiated from **netImgui server** application
 - GUI and save file support for Client configurations
 - Improved samples
 - Optional commandline parameter to specify Client's address for connection on **netImgui Server** launch
 - Launching a second **netImgui Server** forward commandline option to already running instance
 - **netImgui Server** application accepts Client's address request from a Windows 'named pipe' 
- **Bugfix**
  - Issue of text edition not recognizing special key strokes (navigation, delete, ...)
  
### Older
[Release Notes](https://github.com/sammyfreg/netImgui/blob/master/ReleaseNotes.md)

# Contact
Author can be reached for feedbacks / comments at: sammyfreg (at) gmail . com

# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on **[Dear ImGui](https://github.com/ocornut/imgui)**.

Support of image formats via [**stb_image.h**](https://github.com/nothings/stb/blob/master/stb_image.h) by Sean Barrett (public domain).

Support of Solutions generation via [**Sharpmake**](https://github.com/ubisoft/Sharpmake) by Ubisoft (public domain).

Support of Posix sockets thanks to [Aaron Lieberman](https://github.com/AaronLieberman).

Support of json save file via [**nlohmann json**](https://github.com/nlohmann/json)