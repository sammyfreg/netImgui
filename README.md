 <p align="center"><img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImguiLogo.png" width=128 height=128></p>

# Summary
**NetImgui** is a library to remotely display and control **[Dear ImGui](https://github.com/ocornut/imgui)** menus with an associated **NetImgui Server** application. Designed to painlessly integrate into existing codebase with few changes required. It allows any program using **Dear ImGui** to receives input from a remote PC and then forwards its UI rendering to it (*textures, vertices, indices, draw commands*).

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImgui.png)

# Purpose
Initially created to assist game developers in debugging their game running on a game console or smartphone, from a PC. However, its use extends to wider applications. The only requirements are that a program is using **Dear ImGui** using C++ and available TCP/IP communications.

#### 1. UI access with applications without display or input.
Some applications lack display and inputs access, preventing feedbacks and easy control. It could be a program running on a [Raspberry Pie](https://www.raspberrypi.org/products/ "Raspberry Pie") device, or [Unreal 4](https://www.unrealengine.com "Unreal4") running in server mode, etc. . Using **NetImgui** allows the display of a user interface with full control on your PC while the logic remains on the client application itself.


#### 2. Ease of use
While inputs might be available on a particular device (keyboard, gamepad, ...), it might still not be convenient for certain aspect. For example, your smartphone might offer text input, but a full screen window with a physical keyboard is still more comfortable. Idem for a gaming console with gamepad control or VR development.


#### 3. Declutter display
**Dear ImGui** is often used to display relevant debug information during development, but UI elements can obscure the regular content. **NetImgui** sends the UI content to a remote window, leaving the original display clutter-free and with the freedom to use the entire screen for more elaborate content.

###### Before
![DearImGui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/AppWithoutNetImgui.png)

###### With NetImgui
![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/AppWithNetImguiGif.gif)

# How it works
Here is a quick overview of the logic behind using the **NetImgui Server** and one (or more) program using the **NetImgui Client** code.

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/NetImguiExplanation.gif)

**1.** *(NetImgui Server)* **Capture user's inputs with mouse/keyboard**
**2.** *(NetImgui Server)* **Send the Inputs to client and request a draw update**
**3.** *(NetImgui Client)* **Draw the *Dear ImGui* content normally** (without need to display on client) 
**4.** *(NetImgui Client)* **Send the drawing results to server**
**5.** *(NetImgui Server)* **Receives the drawing result and display them**
**6. Repeat the process**

#### Note
The NetImgui Server application currently compiles under Windows, but few changes are required to properly have it running under other Operating Systems.

# Integration
- Download the [latest version](https://github.com/sammyfreg/netImgui/releases "latest version") of the **NetImgui** library.
- Add the content of ***Code\Client*** to your codebase.
- In your codebase :
  - [once] Call `NetImgui::ConnectToApp()` or `NetImgui::ConnectFromApp()`.
  - [Every Redraw]
    - Replace call to `ImGui::NewFrame()` with `NetImgui::NewFrame()`.
    - Draw your ImGui menu as usual.
    - Replace call to `ImGui::Render()` and `ImGui::EndFrame()` with `NetImgui::EndFrame()`.
- Start the **NetImgui** server application and connect your application to it

- *More integration details can be found on the [Wiki](https://github.com/sammyfreg/netImgui/wiki "Wiki"). Multiple samples are also included, providing additional insights*

#### Note
- Connection between **NetImgui Server** and a **netImGui Client** can be achieved in 4 different ways.
 - **Server waits for connection and** :
   - (A) Client calls `ConnectToApp()` with the Server address.
 - **Client calls `ConnectFromApp()` then waits for connection and** :
   - (B) Server configure the Client's address and connect to it.
   - (C) Server is launched with the Client's address in the commandline.
   - (D) Server receives a Client's address from another application, through [Windows named pipe](https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipes "Windows named pipe") : \\.\pipe\netImgui'.
 
 
- **Advanced:** Different **Dear ImGui** content can be displayed locally and remotely at the same time. Look at *SampleDualUI* for more information.

- `NetImgui::IsConnected()` and `NetImgui::IsDrawingRemote()` can be used during Dear ImGui drawing, helping to make selective decisions on the content to draw based on where it will be displayed.

#### Dear Imgui versions
- Tested against **Dear ImGui** versions: **1.74, 1.75, 1.76, 1.76** (docking)**, 1.77, 1.78, 1.79, 1.80, 1.80** (docking).
- *Note*: Should support other versions without too much difficulties.

# Related
Related projects making use of **NetImgui**.
- **[Unreal ImGui](https://github.com/segross/UnrealImGui/ "UnrealImGui") :** Unreal4 Plugin adding **Dear ImGui** support and allowing to display ImGui content over a game or editor. The **net_imgui** branch of this repository contains an integration of the **NetImgui** library for remote access and is ready to go.
- **[Unreal NetImgui](https://github.com/sammyfreg/UnrealNetImgui "UnrealNetImgui")** : Unreal4 plugin also adding access to **Dear ImGui** and **NetImgui**. Unlike the previous plugin, it is a simple implementation with only remote access possible (no ImGui drawing over the game).

# Release notes
### To do
- Support of additional texture formats
- Commands to assign custom backgrounds
- Handle Linear/sRGB vertex color format
- Add logging information in netImgui server application
- Profile and optimize performances
- Add new **Dear ImGui** multi windows support (docking branch)
- ~~Bugfix : netImgui Server application unable to start when post configured for listening can't be opened. Need to manually edit `netImgui.cfg` with a valid port.~~
- ~~Add copy/paste support~~
- ~~Networking: Add support of client accepting connection from netImgui App~~

### Version 1.3
  
### Older
[Release Notes](RELEASENOTES.md)

# Contact
Author can be reached for feedbacks / comments at: sammyfreg (at) gmail . com or through **[GitHub Issues](https://github.com/sammyfreg/netImgui/issues "GitHub Issues")** report.

# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on **[Dear ImGui](https://github.com/ocornut/imgui)**.

Support of image formats via [**stb_image.h**](https://github.com/nothings/stb/blob/master/stb_image.h) by Sean Barrett (public domain).

Support of Solutions generation via [**Sharpmake**](https://github.com/ubisoft/Sharpmake) by Ubisoft (public domain).

Support of Posix sockets thanks to [Aaron Lieberman](https://github.com/AaronLieberman).

Support of json save file via [**nlohmann json**](https://github.com/nlohmann/json)