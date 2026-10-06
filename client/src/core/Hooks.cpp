#include "Hooks.h"
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

void Hooks::shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
