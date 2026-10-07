#include "ActorList.h"
#include "core/Logger.h"
#include <Windows.h>

/*
 * Every actor in the level
 * ------------------------
 * Level has a virtual getRuntimeActorList() that returns std::vector<Actor*> by value.
 * A function that returns a class by value takes a hidden pointer to the result as its
 * first argument after `this`, so we pass an empty vector-shaped struct and the game
 * fills it in.
 *
 * The vector's buffer was allocated by the game's CRT (ucrtbase.dll). This DLL links the
 * CRT statically, so its own free() would use a different heap - we give the buffer back
 * with ucrtbase's free instead.
 */

namespace {
    // MSVC std::vector layout
    struct GameList {
        Actor** first;   // first element
        Actor** last;    // one past the last element
        Actor** end;     // end of the allocation (unused)
    };

    using FreeFunction = void (*)(void* memory);

    FreeFunction findGameFree() {
        HMODULE ucrt = GetModuleHandleW(L"ucrtbase.dll");
        if (ucrt == nullptr) return nullptr;
        return reinterpret_cast<FreeFunction>(GetProcAddress(ucrt, "free"));
    }

    // Logged once, to check that VIndex::getRuntimeActorList still points at the right function
    void logFunctionAddress(void* level) {
        void** vtable = *static_cast<void***>(level);
        void* function = vtable[Offsets::Level::VIndex::getRuntimeActorList];
        LOG("Level vtable = exe+%#llx", Memory::rva(vtable));
        LOG("getRuntimeActorList = exe+%#llx", Memory::rva(function));
    }

    void logResult(const std::vector<Actor*>& list, Actor& player) {
        bool foundMyself = false;
        for (Actor* actor : list) {
            if (actor == &player) foundMyself = true;
        }
        LOG("actor list: %zu actors, contains LocalPlayer = %d", list.size(), foundMyself);
    }
}

void ActorList::get(Actor& player, std::vector<Actor*>& result) {
    result.clear();

    void* level = player.at<void*>(Offsets::Actor::level);
    if (level == nullptr) return;

    static FreeFunction gameFree = findGameFree();   // looked up once
    if (gameFree == nullptr) return;

    static bool firstTime = true;
    if (firstTime) logFunctionAddress(level);

    // The game fills gameList (hidden return-value pointer, see above)
    GameList gameList{};
    Memory::callVirtual<GameList*, GameList*>(level, Offsets::Level::VIndex::getRuntimeActorList, &gameList);

    const size_t count = gameList.last - gameList.first;
    result.assign(gameList.first, gameList.first + count);

    // The buffer belongs to the game's heap: free it there
    if (gameList.first != nullptr) gameFree(gameList.first);

    if (firstTime) {
        logResult(result, player);
        firstTime = false;
    }
}
