@echo off

:: Detect if user has fetched previous version of DearImgui to also generate some compatibility compiling test
set CompatibilityCS=@"./Build/nocompatibility.sharpmake.cs"
if exist "_generated\imgui\compatibility.sharpmake.cs" set CompatibilityCS=@"_generated/imgui/compatibility.sharpmake.cs"

.\Build\Sharpmake\Sharpmake.Application.exe /sources(@"./Build/shared.sharpmake.cs", %CompatibilityCS%, @"./Build/netImgui.sharpmake.cs")

pause