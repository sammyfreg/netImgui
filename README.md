
# netImgui
**[Dear ImGui](https://github.com/ocornut/imgui) remote connection library**

## Summary
**NetImgui** is a library to remotely display and control **Dear Imgui** menus. Designed to painlessly integrate into existing codebase, with few changes required to existing **Dear Imgui** code. 

## Purpose
Initially created to allow  game developpers to easily debug their game from a PC while running on a game console. However, its use can easily extend beyong this.

#### 1. Inputs ease of use
A program might not be running on a platform with access to a keyboard and mouse. With **netImgui** you have the confort of your PC's inputs while controlling your remote program.
<table >
  <tr>
    <th>Dear ImGui alone</th>
    <th>With netImgui</th>
  </tr>
  <tr>
    <td>![DearImGui](https://github.com/sammyfreg/netImgui/blob/master/Web/img/AppWithoutNetImgui.png)</td>
    <td>![netImGui](https://github.com/sammyfreg/netImgui/blob/master/Web/img/AppWithNetImguiGif.gif)</td>
  </tr>
</table>

#### 2. Declutter screen
**Dear Imgui** is often used to display relevant debug informations during developement, but its UI elements might obscure the regular window content. **NetImgui** send the debug menus UI to a separate window, leaving the original display free of clutter and with more freedom to create debug content covering the entire screen.

## Integration
- Download the latest version and compile netImguiServer application
- Either compile the **netImguiLib** library or copy the content of ***Code\Client*** to your codebase
- In your codebase :
 - [once] Call ***NetImgui::Connect*** (starts connection to **netImguiServer**)
 - [Every Redraw]
   - Call ***NetImgui::NewFrame*** (instead of *ImGui::NewFrame*)
   - Draw your ImGui menu as usual.
   - Call **NetImgui::EndFrame** (instead of *ImGui::EndFrame*)
- Look at the ***Sample*** project for me details on changes needed in your codebase

#### Note
- You can display local *'Dear Imgui'* content while simultaneously having other content sent to **netImgui**
- ***NetImgui::IsConnected*** and ***NetImgui::IsRemoteDraw*** can be used during the menu drawing udpate, to modify the *'Dear Imgui'* content

## Other
Thanks to [Omar Cornut](https://github.com/ocornut/imgui/commits?author=ocornut) for developing **Dear ImGui**.
