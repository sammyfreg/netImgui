 <p align="center"><img src="https://github.com/sammyfreg/netImgui/blob/master/Web/img/netImguiLogo.png" width=128 height=128></p>

# Summary
**NetImgui** is a library to remotely display and control **[Dear ImGui](https://github.com/ocornut/imgui)** menus with an associated **netImgui server** application. Designed to painlessly integrate into existing codebase with few changes required to the existing logic.

![NetImgui](https://github.com/sammyfreg/netImgui/blob/master/Web/img/netImgui.png)

# Purpose
Initially created to assist game developers in debugging their game from a PC while it runs on console. However, its use can easily be extended to other fields.

#### 1. Input ease of use
For programs running on hardware without easy access to a keyboard and mouse, **netImgui** provides the  comfort of your computer's inputs while controlling it remotely.

![NetImgui](https://github.com/sammyfreg/netImgui/blob/master/Web/img/InputWithNetImgui.gif)

#### 2. Declutter display
**Dear ImGui** is often used to display relevant debug information during development, but its UI elements can obscure the regular window content. **NetImgui** sends the debug menus to a separate window, leaving the original display clutter-free and with freedom to use the entire screen for more elaborate debug content.

###### Before
![DearImGui](https://github.com/sammyfreg/netImgui/blob/master/Web/img/AppWithoutNetImgui.png)

###### With netImgui
![netImGui](https://github.com/sammyfreg/netImgui/blob/master/Web/img/AppWithNetImguiGif.gif)

# Integration
- Download the latest version of netImgui.
- Generate solutions and compile the **netImguiServer** application.
- Add the content of ***Code\Client*** to your codebase.
- In your codebase :
  - [once] Call ***NetImgui::Connect(...)*** (starts connection to **netImguiServer**).
  - [once] Call ***NetImgui::SendDataTexture(...)*** (upload your font texture to **netImguiServer**).
  - [Every Redraw]
    - Replace ***ImGui::NewFrame()*** call, with ***NetImgui::NewFrame()***.
    - Draw your ImGui menu as usual.
    - Replace ***ImGui::Render()*** call, with ***NetImgui::EndFrame()***.

(More integration details can be found on the [Wiki](https://github.com/sammyfreg/netImgui/wiki "Wiki") and *netImgui_Sample* project can provides insights)

#### Note
- Different **Dear ImGui** content can be displayed locally and remotely at the same time.
- *NetImgui::IsConnected* and *NetImgui::IsRemoteDraw* can be used during menu udpate, to make decisions on what content to draw.

# Release information
### Version 1.1
(2020/23)
- **Changes**
 - Few fixes
 - Added support to Posix sockets (Unix/Mac/Android support)
 - Added texture format A8
 
### Version 1.0
(2020/06)
- **Changes**
 - Initial Release
 - netImgui server application
 - netImgui client code (for integration into external codebase)

- **To do**
 - Support of addional texture formats
 - Commands to assign custom backgrounds
 - Handle Linear/sRGB vertex color format
 - Add logging information in netImgui server application
 - Profile and optimize performances
 - Add copy/past support
 - Add new **Dear ImGui** multi windows support (docking branch)
 - Support of data packet compression

[Release Notes](https://github.com/sammyfreg/netImgui/blob/master/Web/ReleaseNotes.md)

# Contact
Author can be reach for feedback / comments at: zenengine3d (at) gmail . com

# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on **[Dear ImGui](https://github.com/ocornut/imgui)**.

Support of image formats via [**stb_image.h**](https://github.com/nothings/stb/blob/master/stb_image.h) by Sean Barrett (public domain).

Support of Solutions generation via [**Sharpmake**](https://github.com/ubisoft/Sharpmake) by Ubisoft (public domain).
