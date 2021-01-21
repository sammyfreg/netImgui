# Release notes

### Version 1.3
(2021/01/21)
- **API Changes**
  - Removed parameter from `NetImGui::ConnectToApp() / NetImGui::::ConnectFromApp()` to clone the current context. **NetImgui** now only uses the **Dear ImGui** Context that was active when requesting a connection, without internally using other contexts.

- **New**  
  - Complete refactor of the **NetImguiServer** application. 
    - Now relying on **Dear ImGui**'s backend for the renderer and OS support.
    - Other platform's specific functions have all been cleanly abstracted.
    - This means that porting the server application to other platform should be straightforward.
    - Copy/paste from Server to Client support. From Client to Server is still up to the user engine.
  - Complete refactor of the **NetImguiServer** application UI. 
    - Now relying on 'Dear ImGui' docking branch to draw the user interface.
    - Can display multiple connected client's windows at the same time, and they can be docked / moved around as user sees fit.
    - Can now specify a display refresh rate for the connected clients.
  - Single header include support. 
    - For user wanting to minimize changes to their project, it is now possible to only include `NetImgui_Api.h` after declaring the define `NETIMGUI_IMPLEMENTATION`, and all needed source files will also be included for compilation.
  
### Version 1.2
(2020/08/22)
 - **API Changes**
  - `NetImGui::NewFrame()` / `NetImGui::EndFrame()` should always be used, even when disconnected
  - `NetImGui::ConnectToApp() / NetImGui::::ConnectFromApp()` take a new parameter, telling **netImgui** to continue using the same context or use a duplicate
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
------------
### Version 1.1.001
(2020/07/02)
- Added ImGui 1.77 support

### Version 1.1
(2020/06/23)
- Few fixes
- Added support to Posix sockets (Unix/Mac/Android support)
- Added texture format A8
------------ 
### Version 1.0
(2020/06/13)
- Initial Release
- netImgui server application
- netImgui client code (for integration into external codebase)
