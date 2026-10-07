#include "Overlay.h"
#include "core/InputFocus.h"
#include "gui/Menu.h"
#include <imgui.h>
#include <imgui_impl_win32.h>

void Overlay::init(HWND window) {
    ImGui::CreateContext();
    Menu::loadFonts();
    ImGui_ImplWin32_Init(window);
}

void Overlay::buildFrame() {
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::GetIO().MouseDrawCursor = InputFocus::menuOpen;
    if (InputFocus::menuOpen) ClipCursor(nullptr);   // free the mouse while the menu is open
    Menu::render();
    ImGui::Render();
}

void Overlay::shutdown() {
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
