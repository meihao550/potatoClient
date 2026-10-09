#include "Input.h"
#include "Menu.h"
#include "core/Hooks.h"
#include "core/InFlight.h"
#include "core/InputFocus.h"
#include "core/Logger.h"
#include "modules/ModuleManager.h"
#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {
    std::atomic<HWND> g_hwnd = nullptr;
    std::atomic<WNDPROC> g_original = nullptr;
    // Set when we unload but could not leave the WndProc chain: only forward messages from then on
    std::atomic<bool> g_passThrough = false;

    // The game reads mouse movement for the camera through raw input
    // (WM_INPUT -> GetRawInputData). While the menu is open we hand it
    // packets with no movement / no key so the view and player stay still.
    using GetRawInputData_t = UINT(WINAPI*)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
    GetRawInputData_t oGetRawInputData = nullptr;

    UINT WINAPI hkGetRawInputData(HRAWINPUT raw, UINT command, LPVOID data, PUINT size, UINT headerSize) {
        InFlight::Guard guard;   // see core/InFlight.h
        const UINT result = oGetRawInputData(raw, command, data, size, headerSize);
        if (InputFocus::overlayHasInput() && command == RID_INPUT && data && result != static_cast<UINT>(-1) && result >= sizeof(RAWINPUTHEADER)) {
            auto* input = static_cast<RAWINPUT*>(data);
            if (input->header.dwType == RIM_TYPEMOUSE) {
                input->data.mouse.lLastX = 0;
                input->data.mouse.lLastY = 0;
                // keep button releases so nothing stays "held" after the menu closes
                input->data.mouse.usButtonFlags &= RI_MOUSE_LEFT_BUTTON_UP | RI_MOUSE_RIGHT_BUTTON_UP |
                    RI_MOUSE_MIDDLE_BUTTON_UP | RI_MOUSE_BUTTON_4_UP | RI_MOUSE_BUTTON_5_UP;
                input->data.mouse.usButtonData = 0;
            } else if (input->header.dwType == RIM_TYPEKEYBOARD && !(input->data.keyboard.Flags & RI_KEY_BREAK)) {
                input->data.keyboard.MakeCode = 0;
                input->data.keyboard.VKey = 0xFF;   // "no key"
            }
        }
        return result;
    }

    LRESULT CALLBACK hookedWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        InFlight::Guard guard;   // see core/InFlight.h
        if (g_passThrough) return CallWindowProcW(g_original, hwnd, msg, wp, lp);
        if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
            const bool firstPress = !(lp & (1 << 30));   // ignore auto-repeat
            const int vk = static_cast<int>(wp);
            if (firstPress) {
                if (Menu::onKeyForBinding(vk)) return 0;
                if (vk == VK_INSERT) { InputFocus::menuOpen = !InputFocus::menuOpen; return 0; }
                if (vk == VK_END) { Input::unloadRequested = true; return 0; }
                if (vk == VK_HOME) { Menu::openCommandBar(); return 0; }
                if (vk == VK_ESCAPE && InputFocus::commandBarOpen) { Menu::closeCommandBar("Esc"); return 0; }
                if (!InputFocus::overlayHasInput()) ModuleManager::onKey(vk);
            }
        }

        // Key releases always go to ImGui, even with the overlay closed. Otherwise a key
        // released right after closing (the Enter of a command) stays "held" inside ImGui.
        if (!InputFocus::overlayHasInput() && ImGui::GetCurrentContext() && (msg == WM_KEYUP || msg == WM_SYSKEYUP))
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);

        if (InputFocus::overlayHasInput() && ImGui::GetCurrentContext()) {
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
            // Swallow mouse/keyboard so the game doesn't react while the menu is open
            const bool isMouse = msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST;
            const bool isKey = msg >= WM_KEYFIRST && msg <= WM_KEYLAST;
            const bool isRelease = msg == WM_KEYUP || msg == WM_SYSKEYUP ||
                msg == WM_LBUTTONUP || msg == WM_RBUTTONUP || msg == WM_MBUTTONUP || msg == WM_XBUTTONUP;
            if ((isMouse || isKey) && !isRelease) return DefWindowProcW(hwnd, msg, wp, lp);
        }
        return CallWindowProcW(g_original, hwnd, msg, wp, lp);
    }
}

bool Input::init() {
    void* target = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetRawInputData"));
    return Hooks::create("user32!GetRawInputData", target, &hkGetRawInputData, oGetRawInputData);
}

void Input::install(HWND hwnd) {
    if (g_hwnd) return;
    g_hwnd = hwnd;
    // Store the original before switching: a message can arrive the moment we are installed
    g_original = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(hookedWndProc));
}

bool Input::uninstall() {
    const HWND hwnd = g_hwnd.exchange(nullptr);
    if (!hwnd || !g_original) return true;
    // Only undo our subclass while we are still the outermost WndProc. If another overlay
    // (Discord, Steam, ...) subclassed after us, it keeps calling into us: restoring would
    // cut it off, and freeing the DLL would make it call freed memory.
    const auto current = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
    if (current == hookedWndProc) {
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_original.load()));
        return true;
    }
    LOG("warning: another program subclassed the game window after us; staying in its WndProc chain");
    g_passThrough = true;
    return false;
}
