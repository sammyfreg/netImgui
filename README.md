# Summary
**NetImgui** is a library to remotely display and control **[Dear ImGui](https://github.com/ocornut/imgui)** menus with an associated **netImgui server** application. Designed to painlessly integrate into existing codebase with few changes required to display the UI remotely rather than locally on screen.

# Purpose
Initially created to assist game developpers in debuging their game from a PC while it runs on console. However, its use can easily extend beyong this.

**1. Inputs ease of use**
A program might not be running on a platform with access to a keyboard and mouse. With **netImgui** you have the confort of your computer's inputs while controlling your program remotely.

***Dear ImGui***
![DearImGui](https://github.com/sammyfreg/netImgui/blob/master/Web/img/InputIssues.gif)

***With netImgui***
...

**2. Declutter screen**
**Dear ImGui** is often used to display relevant debug informations during developement, but its UI elements might obscure the regular window content. **NetImgui** send the debug menus to a separate window, leaving the original display free of clutter and with freedom to use the entire screen for more elaborate content.

***Dear ImGui***
![DearImGui](https://github.com/sammyfreg/netImgui/blob/master/Web/img/AppWithoutNetImgui.png)

***With netImgui***
![netImGui](https://github.com/sammyfreg/netImgui/blob/master/Web/img/AppWithNetImguiGif.gif)

# Integration
- Download the latest version and compile the **netImguiServer** application.
- Add the content of ***Code\Client*** to your codebase.
- In your codebase :
  - [once] Call ***NetImgui::Connect*** (starts connection to **netImguiServer**).
  - [Every Redraw]
    - Call ***NetImgui::NewFrame*** (instead of *ImGui::NewFrame*).
    - Draw your ImGui menu as usual.
    - Call **NetImgui::EndFrame** (instead of *ImGui::EndFrame*).

(More integration details can be found on the [Wiki](https://github.com/sammyfreg/netImgui/wiki "Wiki")).

#### Note
- It is possible to have both local and remote **Dear ImGui** content displayed simultaneously.
- ***NetImgui::IsConnected*** and ***NetImgui::IsRemoteDraw*** can be used during menu udpate, to modify its content.

# Other
Thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for developing **Dear ImGui**.
