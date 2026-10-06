#pragma once
#include "Actor.h"
#include <functional>

// Hooks Actor::normalTick of LocalPlayer (client) and ServerPlayer (the built-in
// server of a single-player world) so we can run code on each side's own thread.
namespace PlayerTick {
    enum class Side { Client, Server };

    bool init();
    // True while players of that side are ticking (Client: in a world and not paused,
    // Server: the world runs on this PC, i.e. single-player / hosting)
    bool ticking(Side side);
    // Runs fn during the next tick of a player on that side, on that side's thread.
    // fn returns false to say "not this player" - it is then offered to the next
    // player that ticks, and dropped after a couple of seconds.
    void run(Side side, std::function<bool(Actor&)> fn);

    // Runs fn on the server thread with OUR ServerPlayer: the one standing where the
    // LocalPlayer is (other ServerPlayers are LAN guests). Single-player worlds only.
    void runOnOwnServerPlayer(std::function<void(Actor& server)> fn);

    // Runs fn with whichever player owns our real position: our ServerPlayer when the
    // world runs on this PC (server = true), otherwise our LocalPlayer (remote server).
    void runOnSelf(std::function<void(Actor& player, bool server)> fn);

    // Called after every LocalPlayer tick on the client thread (modules like Fly hook in here)
    void setClientTickListener(void (*listener)(Actor& player));
}
