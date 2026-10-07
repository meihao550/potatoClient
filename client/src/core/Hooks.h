#pragma once

namespace Hooks {
    bool init();
    // Creates + enables a MinHook detour. `original` receives the trampoline.
    bool create(const char* name, void* target, void* detour, void** original);
    void shutdown();   // disables and removes every hook

    // Typed version: the detour and the trampoline pointer must have the same function type,
    // so a mismatch is a compile error instead of a crash.
    //   Present_t oPresent = nullptr;
    //   Hooks::create("IDXGISwapChain::Present", target, &hkPresent, oPresent);
    template <class Fn>
    bool create(const char* name, void* target, Fn detour, Fn& original) {
        return create(name, target, reinterpret_cast<void*>(detour), reinterpret_cast<void**>(&original));
    }
}
