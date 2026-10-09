#pragma once
#include <atomic>

// Whether our overlay (module menu / command bar) currently owns the keyboard and mouse.
// It lives in core/ so modules, hooks and the GUI can all ask without depending on each other.
// Written on the window thread (keys) and the render thread (close button), read everywhere.
namespace InputFocus {
    inline std::atomic<bool> menuOpen = false;         // module menu (Insert)
    inline std::atomic<bool> commandBarOpen = false;   // command bar (Home)

    // While true, keyboard/mouse go to our overlay instead of the game
    inline bool overlayHasInput() { return menuOpen || commandBarOpen; }
}
