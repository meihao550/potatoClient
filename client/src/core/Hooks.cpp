#include "Hooks.h"
#include "InFlight.h"
#include "Logger.h"
#include <MinHook.h>

bool Hooks::init() {
    const MH_STATUS s = MH_Initialize();
    if (s != MH_OK && s != MH_ERROR_ALREADY_INITIALIZED) {
        LOG("MH_Initialize failed: %s", MH_StatusToString(s));
        return false;
    }
    return true;
}

bool Hooks::create(const char* name, void* target, void* detour, void** original) {
    if (!target) { LOG("hook %s: target is null, skipped", name); return false; }
    MH_STATUS s = MH_CreateHook(target, detour, original);
    if (s == MH_OK) s = MH_EnableHook(target);
    LOG("hook %-28s @ %p -> %s", name, target, MH_StatusToString(s));
    return s == MH_OK;
}

bool Hooks::shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);   // no new calls reach our detours from here on
    // Calls that already entered a detour still run our code (and return through the
    // trampolines MH_Uninitialize frees), so wait for them first.
    const bool idle = InFlight::waitUntilIdle(3000);
    if (!idle) {
        LOG("warning: %d hooked calls still running after 3 s - keeping hooks memory alive", InFlight::count.load());
        return false;
    }
    Sleep(50);   // the last guard is released a few instructions before the detour returns
    MH_Uninitialize();
    return true;
}
