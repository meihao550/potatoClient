cmake_minimum_required(VERSION 3.20)
project(PotatoClient LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static CRT so the DLL has no runtime dependencies inside the game process
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG        master)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9)
FetchContent_MakeAvailable(minhook)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)

file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS client/src/*.cpp)
add_library(client SHARED ${CLIENT_SOURCES})
target_include_directories(client PRIVATE client/src)
target_compile_definitions(client PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_compile_options(client PRIVATE /utf-8 /W3 /Zi)
# PDB for Release too, so crash offsets in the Windows event log can be mapped to source lines
target_link_options(client PRIVATE /DEBUG /OPT:REF /OPT:ICF)
target_link_libraries(client PRIVATE minhook imgui d3d11 d3d12 dxgi)
