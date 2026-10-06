#pragma once

namespace Hooks {
    bool init();
    // Creates + enables a MinHook detour. `original` receives the trampoline.
    bool create(const char* name, void* target, void* detour, void** original);
    void shutdown();   // disables and removes every hook
}
