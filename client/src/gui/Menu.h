#pragma once
#include "core/InputFocus.h"

// Whether the menu / command bar are open lives in core/InputFocus.h.
namespace Menu {
    // Old names, still used by modules/ until they switch to InputFocus
    inline std::atomic<bool>& open = InputFocus::menuOpen;
    inline bool capturesInput() { return InputFocus::overlayHasInput(); }

    void loadFonts();     // call once after ImGui::CreateContext
    void render();        // call between ImGui::NewFrame / ImGui::Render
    // Returns true if the key was consumed by the keybind editor.
    bool onKeyForBinding(int vk);

    void openCommandBar();
    void closeCommandBar(const char* reason);   // closes once Enter/Esc are released
}
