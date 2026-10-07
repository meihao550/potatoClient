#pragma once
#include <Windows.h>
#include <atomic>

namespace Input {
    inline std::atomic<bool> unloadRequested = false;
    bool init();               // hooks GetRawInputData (camera lock while the menu is open)
    bool hookGameInput();      // hooks GameInput v3 readings; retry until it returns true
    void install(HWND hwnd);   // subclass the game window's WndProc
    // false = someone subclassed after us, so our WndProc must stay (forwarding only) and
    // the DLL must not be freed
    bool uninstall();
}
