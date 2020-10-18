@echo off
echo ================================================================
echo  Generate a sharpmake debug project.
echo  (Helps debugging problem with netImgui project generation)
echo ================================================================
Sharpmake\Sharpmake.Application.exe /sources("shared.sharpmake.cs", "netImgui.sharpmake.cs") /verbose /generateDebugSolution
pause