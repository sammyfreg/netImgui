# Release notes

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
