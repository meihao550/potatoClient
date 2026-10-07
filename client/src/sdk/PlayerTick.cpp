#include "PlayerTick.h"
#include "Actor.h"
#include "core/Hooks.h"
#include "core/InFlight.h"
#include "core/Logger.h"
#include <Windows.h>
#include <atomic>
#include <mutex>
#include <vector>

/*
 * Getting onto the game threads
 * -----------------------------
 * This exe has almost no RTTI and no global pointer to the player, so we start
 * from code: the constructor/destructor of a class writes its vtable into the
 * object (lea reg, [vftable] / mov [obj], reg). A signature on those bytes gives
 * the vtable, and slot 24 of it is normalTick.
 *
 * Hooking normalTick gives us `this` (the player, fresh every tick) and runs us
 * on the right thread, where reading the world and moving players is safe.
 *
 * Why two sides: in a single-player world the game still runs a server inside
 * the same process. It owns the real positions - if only the client player
 * (LocalPlayer) moves, the server moves it straight back. Teleporting the
 * ServerPlayer is what /tp does: the server then tells the client where to go.
 */

namespace {
    using NormalTick_t = void (*)(Actor*);

    struct Task {
        std::function<bool(Actor&)> fn;
        ULONGLONG deadline;
    };

    struct Side {
        const char* name;
        const char* signature;
        void** vtable = nullptr;
        NormalTick_t original = nullptr;
        std::atomic<ULONGLONG> lastTick = 0;
        std::mutex mutex;
        std::vector<Task> tasks;

        void onTick(Actor* self) {
            if (*reinterpret_cast<void***>(self) != vtable) return;   // some other class sharing this function
            const ULONGLONG now = GetTickCount64();
            lastTick = now;

            std::vector<Task> pending;
            {
                std::lock_guard lock(mutex);
                pending.swap(tasks);
            }
            std::vector<Task> keep;
            for (Task& t : pending)
                if (!t.fn(*self) && now < t.deadline) keep.push_back(std::move(t));
            if (keep.empty()) return;
            std::lock_guard lock(mutex);
            tasks.insert(tasks.end(), std::make_move_iterator(keep.begin()), std::make_move_iterator(keep.end()));
        }
    };

    Side g_client{ "LocalPlayer", Offsets::Sig::localPlayerVtable };
    Side g_server{ "ServerPlayer", Offsets::Sig::serverPlayerVtable };

    std::atomic<void (*)(Actor&)> g_clientListener = nullptr;

    Side& get(PlayerTick::Side side) { return side == PlayerTick::Side::Client ? g_client : g_server; }

    void hkClientTick(Actor* self) {
        InFlight::Guard guard;   // see core/InFlight.h
        g_client.original(self);
        g_client.onTick(self);
        if (auto listener = g_clientListener.load(); listener && *reinterpret_cast<void***>(self) == g_client.vtable)
            listener(*self);
    }
    void hkServerTick(Actor* self) {
        InFlight::Guard guard;   // see core/InFlight.h
        g_server.original(self);
        g_server.onTick(self);
    }

    bool install(Side& side, NormalTick_t detour) {
        char name[64];
        snprintf(name, sizeof(name), "%s vtable reference", side.name);
        const uintptr_t hit = Memory::scanOrLog(name, side.signature);
        if (!hit) return false;
        side.vtable = reinterpret_cast<void**>(Memory::resolveRel32(hit, 3, 7));
        LOG("%s vtable at exe+%#llx", side.name, Memory::rva(side.vtable));
        char hookName[64];
        snprintf(hookName, sizeof(hookName), "%s::normalTick", side.name);
        return Hooks::create(hookName, side.vtable[Offsets::Actor::VIndex::normalTick], detour, side.original);
    }
}

bool PlayerTick::init() {
    const bool client = install(g_client, &hkClientTick);
    const bool server = install(g_server, &hkServerTick);
    return client && server;
}

bool PlayerTick::ticking(Side side) {
    const auto& s = get(side);
    return s.original && GetTickCount64() - s.lastTick < 1000;
}

void PlayerTick::run(Side side, std::function<bool(Actor&)> fn) {
    auto& s = get(side);
    std::lock_guard lock(s.mutex);
    s.tasks.push_back({ std::move(fn), GetTickCount64() + 2000 });
}

void PlayerTick::runOnOwnServerPlayer(std::function<void(Actor&)> fn) {
    run(Side::Client, [fn = std::move(fn)](Actor& local) {
        StateVectorComponent* state = local.stateVector();
        if (!state) return true;
        const Vec3 pos = state->pos;
        run(Side::Server, [fn, pos](Actor& server) {
            StateVectorComponent* s = server.stateVector();
            if (!s) return false;
            const float dx = s->pos.x - pos.x, dz = s->pos.z - pos.z;
            if (dx * dx + dz * dz > 8.0f * 8.0f) return false;   // a LAN guest, not us
            fn(server);
            return true;
        });
        return true;
    });
}

void PlayerTick::runOnSelf(std::function<void(Actor&, bool)> fn) {
    if (ticking(Side::Server)) {
        runOnOwnServerPlayer([fn = std::move(fn)](Actor& server) { fn(server, true); });
        return;
    }
    run(Side::Client, [fn = std::move(fn)](Actor& local) { fn(local, false); return true; });
}

void PlayerTick::setClientTickListener(void (*listener)(Actor&)) {
    g_clientListener = listener;
}
