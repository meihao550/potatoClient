#include "ActorList.h"
#include "core/Logger.h"
#include <Windows.h>

namespace{
    struct RawVector {
        Actor** first;
        Actor** last;
        Actor** end;
    };

    using Free_t = void(*)(void *);
    Free_t gameFree() {
        static Free_t fn = reinterpret_cast<Free_t>(GetProcAddress(GetModuleHandleW(L"ucrtbase.dll"), "free"));
        return fn;
    }
}

std::vector<Actor*> ActorList::get(Actor& player) {
    std::vector<Actor*> result;
    void* level = player.at<void*>(Offsets::Actor::level);
    if(!level || !gameFree()) return result;

    static bool logged = false;
    if (!logged) {
        void ** vtable = *static_cast<void***>(level);
        LOG("Level vtable exe+%#llx, getRuntimeActorList = exe +%#llx",
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(vtable) - Memory::moduleBase()),
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(vtable[Offsets::Level::VIndex::getRuntimeActorList]) - Memory::moduleBase()));
    }

    RawVector raw{};
    Memory::callVirtual<RawVector*, RawVector*>(level, Offsets::Level::VIndex::getRuntimeActorList, &raw);
    if (raw.first) {
        result.assign(raw.first, raw.last);
        gameFree()(raw.first);
    }

    if (!logged) {
        bool hasSelf = false;
        for (Actor* a : result) hasSelf |= (a == &player);
        LOG("actor list: %zu actors, contains LocalPlayer = %d", result.size(), hasSelf);
        logged = true;
    }
    return result;
}