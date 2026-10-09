#include "PlayerTick.h"
#include "Actor.h"
#include "core/Hooks.h"
#include "core/InFlight.h"
#include "core/Logger.h"
#include <Windows.h>
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <utility>
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

bool PlayerTick::hooked(Side side) { return get(side).original != nullptr; }

bool PlayerTick::ticking(Side side) {
    const auto& s = get(side);
    return s.original && GetTickCount64() - s.lastTick < 1000;
}

void PlayerTick::run(Side side, std::function<bool(Actor&)> fn) {
    auto& s = get(side);
    std::lock_guard lock(s.mutex);
    s.tasks.push_back({ std::move(fn), GetTickCount64() + 2000 });
}

namespace {
    // (player, squared distance to our LocalPlayer), measured when the player ticked
    using SeenPlayer = std::pair<Actor*, float>;
    using Seen = std::vector<SeenPlayer>;
    using Pick = Actor* (*)(Seen& seen);

    bool closerFirst(const SeenPlayer& a, const SeenPlayer& b) { return a.second < b.second; }
    bool contains(const Seen& seen, const Actor* player) {
        for (const SeenPlayer& s : seen)
            if (s.first == player) return true;
        return false;
    }

    // Server thread: every ServerPlayer ticks once per round, so a task that sees the same player
    // twice has seen them all. pick then chooses one by distance to our LocalPlayer, and fn runs
    // during that player's next tick (never through a stored pointer: they may have left meanwhile).
    // fn gets nullptr when pick finds nobody.
    void runOnPicked(Pick pick, std::function<void(Actor*)> fn) {
        PlayerTick::run(PlayerTick::Side::Client, [pick, fn = std::move(fn)](Actor& local) {
            StateVectorComponent* state = local.stateVector();
            if (!state) return true;
            const Vec3 ours = state->pos;
            struct Round { Seen seen; Actor* target = nullptr; bool picked = false; };
            auto round = std::make_shared<Round>();
            PlayerTick::run(PlayerTick::Side::Server, [pick, fn, ours, round](Actor& server) {
                if (!round->picked) {
                    auto& seen = round->seen;
                    if (!contains(seen, &server)) {
                        StateVectorComponent* s = server.stateVector();
                        if (s) {
                            const float dx = s->pos.x - ours.x, dy = s->pos.y - ours.y, dz = s->pos.z - ours.z;
                            seen.push_back({ &server, dx * dx + dy * dy + dz * dz });
                        }
                        return false;
                    }
                    round->picked = true;
                    std::sort(seen.begin(), seen.end(), closerFirst);
                    round->target = pick(seen);
                    if (!round->target) { fn(nullptr); return true; }
                }
                if (&server != round->target) return false;
                fn(&server);
                return true;
            });
            return true;
        });
    }
}

void PlayerTick::runOnOwnServerPlayer(std::function<void(Actor&)> fn) {
    // Ours is the one standing where the LocalPlayer is: the closest (a guest can stand right next to us)
    runOnPicked([](Seen& seen) { return seen.empty() ? nullptr : seen[0].first; },
                [fn = std::move(fn)](Actor* server) { if (server) fn(*server); });
}

void PlayerTick::runOnNearestGuest(std::function<void(Actor* guest)> fn) {
    runOnPicked([](Seen& seen) {
        constexpr float range = 16.0f;
        return seen.size() >= 2 && seen[1].second <= range * range ? seen[1].first : nullptr;
    }, std::move(fn));
}

void PlayerTick::runOnServerThenClient(std::function<bool(Actor&)> serverFn, std::function<void(Actor&)> clientFn) {
    runOnOwnServerPlayer([serverFn = std::move(serverFn), clientFn = std::move(clientFn)](Actor& server) {
        if (!serverFn(server)) return;
        run(Side::Client, [clientFn](Actor& local) { clientFn(local); return true; });
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
