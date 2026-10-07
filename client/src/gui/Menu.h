#pragma once
#include <atomic>

namespace Menu {
    // Written on the window thread (keys) and the render thread (close button),
    // read by the input hooks and the game thread - hence atomic.
    inline std::atomic<bool> open = false;          // module menu (Insert)
    inline std::atomic<bool> commandOpen = false;   // command bar (Home)
    // While true, keyboard/mouse go to our overlay instead of the game
    inline bool capturesInput() { return open || commandOpen; }

    void loadFonts();     // call once after ImGui::CreateContext
    void render();        // call between ImGui::NewFrame / ImGui::Render
    // Returns true if the key was consumed by the keybind editor.
    bool onKeyForBinding(int vk);

    void openCommandBar();
    void closeCommandBar(const char* reason);   // closes once Enter/Esc are released
}
