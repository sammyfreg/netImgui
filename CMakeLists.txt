cmake_minimum_required(VERSION 3.15)
project(netImgui)
set(CMAKE_CXX_STANDARD 17)


#
# Options for the project (edit or set those)
#
option(NETIMGUI_BUILD_CLIENT "Build client" ON)
option(NETIMGUI_BUILD_IMGUI "Build imgui from sources in netImgui" ${PROJECT_IS_TOP_LEVEL})
option(NETIMGUI_BUILD_SAMPLES "Build samples" ${PROJECT_IS_TOP_LEVEL})
option(NETIMGUI_BUILD_SERVER_LIB "Build server lib" ${PROJECT_IS_TOP_LEVEL})
option(NETIMGUI_BUILD_SERVER_APP "Build server app" ${PROJECT_IS_TOP_LEVEL})

if(NETIMGUI_BUILD_SAMPLES)
    # Set which backend to use for the samples between WIN32_DX11 and GLFW_GL3
    # (select only one backend!)
    # (this backend is used only for the local display, the server app can use a different backend)
    if (NOT WIN32)
        set(not_win32 ON)
    endif()
    option(NETIMGUI_SAMPLES_BACKEND_WIN32_DX11 "Use Win32/DX11 backend for samples" ${WIN32})
    option(NETIMGUI_SAMPLES_BACKEND_GLFW_GL3 "Use GLFW/GL3 backend for samples" ${not_win32})
endif()
if(NETIMGUI_BUILD_SERVER_APP)
    # NETIMGUI_SERVER_APP_BACKEND:
    # Set which backend to use, between SOKOL, WIN32_DX11, and GLFW_GL3
    # By default, we use Sokol
    # (select only one backend!)
    option(NETIMGUI_SERVER_APP_BACKEND_SOKOL "Use Sokol backend for server app" ON)

    # implementation to be completed below
    # option(NETIMGUI_SERVER_APP_BACKEND_WIN32_DX11 "Use Win32/DX11 backend for server app" OFF)
    # option(NETIMGUI_SERVER_APP_BACKEND_GLFW_GL3 "Use GLFW/GL3 backend for server app" OFF)
endif()

#
# Code directories
#
set(CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Code)
set(CLIENT_DIR ${CODE_DIR}/Client)
set(SERVER_DIR ${CODE_DIR}/ServerApp/Source)
set(SAMPLES_DIR ${CODE_DIR}/Sample)
set(IMGUI_DIR ${CODE_DIR}/ThirdParty/DearImgui)
set(COMMON_DIR ${SAMPLES_DIR}/Common)

#
# Add imgui
#
function(add_imgui)
    file(GLOB imgui_files ${IMGUI_DIR}/*.cpp ${IMGUI_DIR}/*.h)
    add_library(imgui ${imgui_files})
    target_include_directories(imgui PUBLIC ${IMGUI_DIR} $<BUILD_INTERFACE:${IMGUI_DIR}/backends>)
    # target_compile_definitions(imgui PUBLIC ImTextureID=uint64_t)
endfunction()

#
# Add netImgui Client: this is the library that will be linked to the application logic.
# The client will sent display commands to the server
#
function(add_net_imgui_client)
    file(GLOB_RECURSE client_files ${CLIENT_DIR}/*.cpp ${CLIENT_DIR}/*.h)
    add_library(net_imgui_client ${client_files})
    target_include_directories(net_imgui_client PUBLIC $<BUILD_INTERFACE:${CLIENT_DIR}>)
    target_link_libraries(net_imgui_client PUBLIC imgui)
endfunction()

#
# Add netImgui Server lib (because several server apps can be created, using different backends)
#
function(add_net_imgui_server_lib)
    file(GLOB server_files ${SERVER_DIR}/*.cpp ${SERVER_DIR}/*.h)
    add_library(net_imgui_server_lib ${server_files})
    target_link_libraries(net_imgui_server_lib PUBLIC net_imgui_client)
    target_include_directories(net_imgui_server_lib PUBLIC $<BUILD_INTERFACE:${SERVER_DIR}> $<BUILD_INTERFACE:${CODE_DIR}>)

    # Add NetImguiServer_App_Custom
    target_sources(net_imgui_server_lib PRIVATE ${SERVER_DIR}/Custom/NetImguiServer_App_Custom.cpp)
endfunction()

#
# Add Server app (depending on the backend)
#
function(_add_net_imgui_server_app_sokol)
    set(server_app_sokol_dir ${SERVER_DIR}/Sokol)
    file(GLOB server_app_sokol_files ${server_app_sokol_dir}/*.cpp)
    add_executable(net_imgui_server_app_sokol ${server_app_sokol_files})
    target_link_libraries(net_imgui_server_app_sokol net_imgui_server_lib)
    # Per platform settings
    if(APPLE)
        target_compile_options(net_imgui_server_app_sokol PRIVATE -x objective-c++)
        target_link_libraries(net_imgui_server_app_sokol "-framework Cocoa" "-framework QuartzCore" "-framework Metal" "-framework MetalKit -framework OpenGL")
    elseif(UNIX)
        target_link_libraries(net_imgui_server_app_sokol X11 GL Xcursor Xi)
    endif()
endfunction()

function(_add_net_imgui_server_app_win32_dx11)
    message(FATAL_ERROR "Not implemented")
endfunction()

function(_add_net_imgui_server_app_glfw_gl3)
    message(FATAL_ERROR "Not implemented")
endfunction()

function(add_net_imgui_server_app)
    # Ensure only one backend is selected
    set(backend_count 0)
    if(NETIMGUI_SERVER_APP_BACKEND_SOKOL)
        math(EXPR backend_count "${backend_count}+1")
    endif()
    if(NETIMGUI_SERVER_APP_BACKEND_WIN32_DX11)
        math(EXPR backend_count "${backend_count}+1")
    endif()
    if(NETIMGUI_SERVER_APP_BACKEND_GLFW_GL3)
        math(EXPR backend_count "${backend_count}+1")
    endif()
    if(NOT backend_count EQUAL 1)
        message(FATAL_ERROR "Select one and only one backend for the server app")
    endif()

    if(NETIMGUI_SERVER_APP_BACKEND_SOKOL)
        target_compile_definitions(net_imgui_server_lib PUBLIC NETIMGUI_SERVER_APP_BACKEND_SOKOL)
        _add_net_imgui_server_app_sokol()
#    elseif(NETIMGUI_SERVER_APP_BACKEND_WIN32_DX11)
#        target_compile_definitions(net_imgui_server_lib PUBLIC NETIMGUI_SERVER_APP_BACKEND_WIN32_DX11)
#        _add_net_imgui_server_app_win32_dx11()
#    elseif(NETIMGUI_SERVER_APP_BACKEND_GLFW_GL3)
#        target_compile_definitions(net_imgui_server_lib PUBLIC NETIMGUI_SERVER_APP_BACKEND_GLFW_GL3)
#        _add_net_imgui_server_app_glfw_gl3()
    endif()
endfunction()

#
# Build all Samples (in a dir whose name starts with "Sample")
#
function(_add_backend_to_sample sample_name)
    # Depending on the backend, we need to
    #    - use different main files (fill main_file)
    #    - link to different libraries (fill link_libraries)
    #    - add different backend impl files (fill backend_files)
    if(NETIMGUI_SAMPLES_BACKEND_WIN32_DX11)
        add_compile_definitions(NETIMGUI_SAMPLES_BACKEND_WIN32_DX11)
        set(main_file ${COMMON_DIR}/main.cpp)
        set(backend_files ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp ${IMGUI_DIR}/backends/imgui_impl_win32.cpp)
        set(link_libraries d3d11)

    elseif(NETIMGUI_SAMPLES_BACKEND_GLFW_GL3)
        find_package(glfw3 REQUIRED)
        find_package(OpenGL REQUIRED)
        add_compile_definitions(NETIMGUI_SAMPLES_BACKEND_GLFW_GL3)
        set(main_file ${COMMON_DIR}/main_glfw_gl3.cpp)
        set(backend_files ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
        set(link_libraries glfw OpenGL::GL)
    endif()

    target_sources(${sample_name} PRIVATE ${main_file} ${backend_files})
    target_link_libraries(${sample_name} PRIVATE ${link_libraries})
endfunction()

function(_add_common_to_sample sample_name)
    target_sources(${sample_name} PRIVATE ${COMMON_DIR}/Sample.h ${COMMON_DIR}/Sample.cpp)
    target_include_directories(${sample_name} PUBLIC $<BUILD_INTERFACE:${CODE_DIR}>)
endfunction()

function(_add_net_imgui_sample sample_name)
    message("Building sample: ${sample_name}")
    set(sample_dir ${SAMPLES_DIR}/${sample_name})
    file(GLOB sample_files ${sample_dir}/*.cpp ${sample_dir}/*.h)
    add_executable(${sample_name} ${sample_files})
    target_link_libraries(${sample_name} PRIVATE net_imgui_client)
endfunction()

function(add_net_imgui_samples)
    file(GLOB samples_list ${SAMPLES_DIR}/Sample*)
    foreach (sample_dir ${samples_list})
        get_filename_component(sample_name ${sample_dir} NAME)

        # SampleCompression is not supported, because 
        # SampleNetworkWin32.cpp is problematic:
        #   - it does not compile outside of windows (need to add #ifdef _WIN32)
        #   - under windows, its presence will lead to link error (doubly defined functions): NetImgui::Internal::Network::Startup(void) already defined, etc.
        if (${sample_name} STREQUAL "SampleCompression")
            continue()
        endif()

        _add_net_imgui_sample(${sample_name})

        # Link backend to sample (if not SampleNoBackend)
        if(NOT ${sample_name} STREQUAL "SampleNoBackend")
            _add_common_to_sample(${sample_name})
            _add_backend_to_sample(${sample_name})
        endif()
    endforeach()
endfunction()


#
# Main
#
if(NETIMGUI_BUILD_IMGUI)
    add_imgui()
endif()
if (NETIMGUI_BUILD_CLIENT)
    add_net_imgui_client()
endif()
if(NETIMGUI_BUILD_SERVER_LIB)
    add_net_imgui_server_lib()
endif()
if(NETIMGUI_BUILD_SERVER_APP)
    add_net_imgui_server_app()
endif()
if(NETIMGUI_BUILD_SAMPLES)
    add_net_imgui_samples()
endif()
