@echo off
setlocal enabledelayedexpansion
cls
pushd %~dp0

::-----------------------------------------------------------------------------------
:: SETTINGS 
::-----------------------------------------------------------------------------------
:: List of offical Dear ImGui (from official depot)
set VERSIONS=(v1.71 v1.72 v1.73 v1.74 v1.75 v1.76 v1.77 v1.78 v1.79, v1.80, v1.81, v1.82, v1.83, v1.84, v1.85, v1.86, v1.87, v1.88, v1.89.1, v1.89.2, v1.89.3, v1.89.4, v1.89.5, v1.89.6, v1.89.7, v1.89.7-docking, v1.89.8, v1.89.8-docking, v1.89.9, v1.89.9-docking, v1.90, v1.90-docking, v1.90.1, v1.90.2, v1.90.3, v1.90.4, v1.90.5, v1.90.6, v1.90.6-docking, v1.90.7, v1.90.8, v1.90.9, v1.91.0, v1.91.0-docking, v1.91.1, v1.91.2, v1.91.3, v1.91.4, v1.91.5, v1.91.5-docking, v1.92.0, v1.92.0-docking)

:: List of custom Dear ImGui releases (from own depot)
set EXTRA_VERSIONS=(dock-1-76, dock-1-80, dock-1-89)

:: Output directory
set IMGUI_DIR=%~dp0..\_generated\imgui

:: Destination of Compatibility project config file
set COMPAT_FILE=%IMGUI_DIR%\compatibility.sharpmake.cs

:: goto SkipDownload

:GetAllImgui
echo ================================================================================
echo  Download multiple versions of Dear Imgui.
echo  Unpack them and generates netImgui projects for each.
echo  Used to test compatibility against multiple version of Dear ImGui
echo ================================================================================
echo.

:: Only download new release

:: if not exist %IMGUI_DIR% goto SkipDelete
:: echo.
:: echo --------------------------------------------------------------------------------
:: echo  Clearing Releases
:: echo --------------------------------------------------------------------------------
:: echo Removing: %IMGUI_DIR%
:: rmdir /s /q %IMGUI_DIR%


:SkipDelete
if not exist %IMGUI_DIR% (
	mkdir %IMGUI_DIR%
)

echo.
echo --------------------------------------------------------------------------------
echo  Downloading and extracting Dear ImGui Releases
echo --------------------------------------------------------------------------------
pushd %IMGUI_DIR%

for %%v in %VERSIONS% do (    
    echo Extracting: %%v
	set IMGUI_FILE=%%v.tar.gz
	set IMGUI_FILEPATH="https://github.com/ocornut/imgui/archive/!IMGUI_FILE!"
	if not exist !IMGUI_FILE! curl -LJ !IMGUI_FILEPATH! --output !IMGUI_FILE!
	tar -xzf !IMGUI_FILE!
)

for %%v in %EXTRA_VERSIONS% do (    
    echo Extracting: %%v
	set IMGUI_FILE=%%v.zip
	set IMGUI_FILEPATH="https://raw.githubusercontent.com/wiki/sammyfreg/netImgui/ImguiVersions/!IMGUI_FILE!"
	if not exist !IMGUI_FILE! curl -LJ !IMGUI_FILEPATH! --output !IMGUI_FILE!
	tar -xzf !IMGUI_FILE!
)
popd

:SkipDownload
echo.
echo --------------------------------------------------------------------------------
echo  Generating Sharpmake config for compatibility projects
echo --------------------------------------------------------------------------------
type compatibility.sharpmake.cs.1 > %COMPAT_FILE%
:: Declare each compatibility project (1 per Imgui version)
for /D %%d IN (%IMGUI_DIR%\*) DO (
	call :GenerateProjectName %%d	
	echo     [Sharpmake.Generate] public class !NetImguiName! : ProjectNoBackend { public !NetImguiName!^(^): base^("!NetImguiName!", @"%%d"^){} } >> %COMPAT_FILE%
)

type compatibility.sharpmake.cs.2 >> %COMPAT_FILE%
:: Create function adding each project
for /D %%d IN (%IMGUI_DIR%\*) DO (
	call :GenerateProjectName %%d
	echo             conf.AddProject^<!NetImguiName!^>^(target, false, SolutionFolder^); >> %COMPAT_FILE%
	echo !NetImguiName! added
)
type compatibility.sharpmake.cs.3 >> %COMPAT_FILE%

echo.
echo --------------------------------------------------------------------------------
echo  Dear ImGui Older versions fetched
echo  Please regenerate the projects to have them included in compiling tests
echo --------------------------------------------------------------------------------
pause

popd
exit /b %errorlevel%

:: Take a Imgui install path, and make it into a NetImgui project name
:: By keeping only the last directory name and removing '-' and '.'
:GenerateProjectName
	set NetImguiName=%~nx1
	set NetImguiName=%NetImguiName:-=_%
	set NetImguiName=%NetImguiName:.=_%
	set NetImguiName=ProjectCompatibility_%NetImguiName%
exit /b 0


