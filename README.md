 <p align="center"><img src="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImguiLogo.png" width=128 height=128></p>


# Summary
(**Branch Master** [![Build Branch: Master](https://github.com/sammyfreg/netImgui/actions/workflows/msbuild_vs2022.yml/badge.svg?branch=master)](https://github.com/sammyfreg/netImgui/actions/workflows/msbuild_vs2022.yml))
(**Branch Dev** [![Build Branch: Dev](https://github.com/sammyfreg/netImgui/actions/workflows/msbuild_vs2022.yml/badge.svg?branch=dev)](https://github.com/sammyfreg/netImgui/actions/workflows/msbuild_vs2022.yml))

**NetImgui** is a library to remotely display and control **[Dear ImGui](https://github.com/ocornut/imgui)** menus with an associated **NetImgui Server** application. Designed to painlessly integrate into existing codebase with few changes required. It allows any program using **Dear ImGui** to receives input from a remote PC and then forwards its UI rendering to it (*textures, vertices, indices, draw commands*).

![NetImgui](https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/Web/img/netImgui1_7.png)

# Purpose
Initially created to assist game developers in debugging their game running on a game console or smartphone, from a PC. However, its use extends to wider applications. The only requirements are that a program is using **Dear ImGui** using C++ and available TCP/IP communications.

#### 1. UI for devices without display/input.
Some applications lack display and inputs access, preventing feedbacks and easy control. It could be a program running on a [Raspberry Pie](https://www.raspberrypi.org/products/ "Raspberry Pie") device, or [Unreal 4](https://www.unrealengine.com "Unreal4") running in server mode, etc . . . Using **NetImgui** allows the display of a user interface with full control on your PC while the logic remains on the client application itself.

**Note**: *SampleNoBackend demonstrate using Dear ImGui without any backend implementation. It is a simple console application connecting to the NetImguiServer to draw its Imgui content. There is no rendering/input backend that's usually require by Dear ImGui*

#### 2. Ease of use
While inputs might be available on a particular device (keyboard, gamepad, . . .), it might still not be convenient for certain aspect. For example, text input is still more comfortable on a physical keyboard than on a smartphone. Idem for gaming consoles with gamepads control or VR development.

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

**6. Go back to 1**

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
- Tested against **Dear ImGui** versions: **1.71, 1.72, 1.73, 1.74, 1.75, 1.76, 1.76** (docking)**, 1.77, 1.78, 1.79, 1.80, 1.80** (docking)**, 1.81, 1.82, 1.83, 1.84, 1.85, 1.86, 1.87, 1.87**.
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

### Version 1.8
(2022/07/18)
- **API Changes**
  - None
- **Dear ImGui 1.87 input event support**
  - Input handling refactored to handle new Dear ImGui input event system (thanks to @lemantisee)
  - Added gamepad support
- **Compatibility tests**
  - Now compile simple compatibility app for each Dear ImGui version supported
  - Allows to make sure the NetImgui Server works properly against any Dear ImGui version
- **Other Changes**
  - 32bits support bugs fix in client code
  - NetImgui server popup now centered on main window
  - Some OpenGL fixes
  - Visual Studio 2022 LLVM support fix

### Older
[Release Notes](https://github.com/sammyfreg/netImgui/wiki/Release-notes)

# Contact
It seems that various gaming studios are making use of this code. I would enjoy hearing back from library users, for general comments and feedbacks. Always interested in learning of people usercase. I can be reached through my  [**GitHub profile**](https://github.com/sammyfreg) (email or twitter) or creating a new entry in [**GitHub Issue**](https://github.com/sammyfreg/netImgui/issues "GitHub Issues"). 

# Credits
Sincere thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for the incredible work on **[Dear ImGui](https://github.com/ocornut/imgui)**.

Support of image formats via [**stb_image.h**](https://github.com/nothings/stb/blob/master/stb_image.h) by Sean Barrett (public domain).

Support of Solutions generation via [**Sharpmake**](https://github.com/ubisoft/Sharpmake) by Ubisoft (public domain).

Support of Posix sockets thanks to [Aaron Lieberman](https://github.com/AaronLieberman).

Support of json save file via [**nlohmann json**](https://github.com/nlohmann/json)