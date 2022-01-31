 <p align="center"><img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImguiLogo.png" width=128 height=128></p>

# Contact
It seems that various gaming studios are making use of this code. I would enjoy hearing back from library users, for general comments and feedbacks. Always interested in learning of people usercase. I can be reached through my  [**GitHub profile**](https://github.com/sammyfreg) (email or twitter) or a opening a [**GitHub Issue**](https://github.com/sammyfreg/netImgui/issues "GitHub Issues"). 


# Summary
**NetImgui** is a library to remotely display and control **[Dear ImGui](https://github.com/ocornut/imgui)** menus with an associated **NetImgui Server** application. Designed to painlessly integrate into existing codebase with few changes required. It allows any program using **Dear ImGui** to receives input from a remote PC and then forwards its UI rendering to it (*textures, vertices, indices, draw commands*).

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImgui1_7.png)

# Purpose
Initially created to assist game developers in debugging their game running on a game console or smartphone, from a PC. However, its use extends to wider applications. The only requirements are that a program is using **Dear ImGui** using C++ and available TCP/IP communications.

#### 1. UI access with applications without display or input.
Some applications lack display and inputs access, preventing feedbacks and easy control. It could be a program running on a [Raspberry Pie](https://www.raspberrypi.org/products/ "Raspberry Pie") device, or [Unreal 4](https://www.unrealengine.com "Unreal4") running in server mode, etc. . Using **NetImgui** allows the display of a user interface with full control on your PC while the logic remains on the client application itself.


#### 2. Ease of use
While inputs might be available on a particular device (keyboard, gamepad, ...), it might still not be convenient for certain aspect. For example, text input is still more comfortable on a physical keyboard than on a smartphone. Idem for gaming consoles with gamepads control or VR development.


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
1. Download the [latest version](https://github.com/sammyfreg/netImgui/releases "latest version") of the **NetImgui** library.
2. Add the content of ***Code\Client*** to your codebase.
3. In your codebase:
   - [once]
     - Call `NetImgui::Startup()` *(at program start)*.
     - Call `NetImgui::ConnectToApp()` or `NetImgui::ConnectFromApp()`.
     - Call `NetImgui::Shutdown()` *(at program exit)*.
   - [every redraw]
     - Draw your ImGui menu as usual.
     - *If **Dear ImGui** 1.80 and lower (or want frameskip support)*.
       - Replace call to `ImGui::NewFrame()` with `NetImgui::NewFrame()`.
       - Replace call to `ImGui::Render()` / `ImGui::EndFrame()` with `NetImgui::EndFrame()`.
4. Start the **NetImgui** server application and connect your application to it.

- *More integration details can be found on the [Wiki](https://github.com/sammyfreg/netImgui/wiki "Wiki").*
- *Multiple samples are also provided in the depot, providing additional insights.*

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
- Tested against **Dear ImGui** versions: **1.74, 1.75, 1.76, 1.76** (docking)**, 1.77, 1.78, 1.79, 1.80, 1.80** (docking)**, 1.81, 1.82, 1.83, 1.84, 1.85, 1.86**.
- *Note*: Should support other versions without too much difficulties.

# Related
Related projects making use of **NetImgui**.
- **[Unreal NetImgui](https://github.com/sammyfreg/UnrealNetImgui "UnrealNetImgui")** : **Unreal 4 plugin**  adding access to **Dear ImGui** and **NetImgui** functionalities. Unlike the next plugin, it is a simple implementation with only remote access possible (no Dear ImGui drawing over the game).
- **[Unreal ImGui](https://github.com/segross/UnrealImGui/ "UnrealImGui") :** **Unreal 4 Plugin** adding **Dear ImGui** drawing support over your game and editor display. The **net_imgui** branch of this repository contains an integration of this **NetImgui** library for remote access and is ready to go.
- For **Unreal 4** developpers with their own **Dear ImGui** integration, **[Unreal NetImgui](https://github.com/sammyfreg/UnrealNetImgui "UnrealNetImgui")** has support for an **Imgui Unreal Commands window** that can be of interest. The code can easily be imported in your own codebase, with no dependency to **Unreal NetImgui** proper.
![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/UnrealCommandsFull.gif)

# Release notes
### To do
- Support of additional texture formats
- Handle Linear/sRGB vertex color format
- Add logging information in netImgui server application
- Profile and optimize performances
- Add new **Dear ImGui** multi windows support (docking branch)
- ~~Commands to assign custom backgrounds~~
- ~~Add compression to data between Client and Server~~

### Version 1.7.5
(2022/01/31)
- **API Changes**
  - None
- **Bug fixes**
  - NetImgui Server memory leak fix
  
### Version 1.7.4
(2022/01/30)
- **API Changes**
  - Removed the `bWait` parameter from `Shutdown()`
      - Always wait for Shutdown completion before returning now
      - This prevents multithread data release issues
  - Added a `NETIMGUI_API` define prefix to functions exported in `NetImgui_Api.h`
      - Similar to `IMGUI_API`, allows library user to control dll import/export
      - Takes the same value as `IMGUI_API` by default
- **Bug fixes**
  - Compression feature causing some out of sync issue with NetImgui Server
  - 32bits index support crash
  
### Version 1.7
(2022/01/10)
- **API Changes**
  - Added `SetCompressionMode(...)` / `GetCompressionMode()`, allow library user to manage the data compression feature
    - By default, Client relies on Server Setting
- **Data Compressions Support**
  - Drawing data generated by the Client can now be compressed before being sent to Server
    - Greatly reduce the bandwidth between Client/Server at a negligible cost on the Client
    - Using Delta Compression. The draw data usually vary little from previous frame. Sending only the changed bytes produce great results at low overhead
  - Highly dynamic UI content lower improvements
  - A new sample (**SampleCompression**) demonstrate the use of data compression and its metrics
    - At 30 Fps, data rate send from client goes from ~ **3400 KB/s** to ~ **22 KB/s** (~ **160x improvement !**)
    - Since it depends on static content, moving around a Window reduce the benefits
- **Other Changes**
  - Added OpenGL3 support to NetImgui Server
    - Still DirectX11 by default
    - OpenGL3 support helps with the Server Application port to non Windows OS
    - To enable, set `HAL_API_PLATFORM_GLFW_GL3` define to 1 and `HAL_API_PLATFORM_WIN32_DX11` to 0
  - Added support for Visual Studio 2021
  - Support up to **Dear ImGui 1.86** has been tested

### Older
[Release Notes](https://github.com/sammyfreg/netImgui/wiki/Release-notes)

# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on **[Dear ImGui](https://github.com/ocornut/imgui)**.

Support of image formats via [**stb_image.h**](https://github.com/nothings/stb/blob/master/stb_image.h) by Sean Barrett (public domain).

Support of Solutions generation via [**Sharpmake**](https://github.com/ubisoft/Sharpmake) by Ubisoft (public domain).

Support of Posix sockets thanks to [Aaron Lieberman](https://github.com/AaronLieberman).

Support of json save file via [**nlohmann json**](https://github.com/nlohmann/json)