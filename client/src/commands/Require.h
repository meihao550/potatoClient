#pragma once
#include "sdk/Actor.h"
#include <optional>

// Checks commands share. Each returns "not OK" after telling the user why (CommandManager::print),
// so a command can just do  if (!Require::inWorld()) return;
namespace Require {
    // Render thread (Command::execute)
    bool inWorld();     // the player is ticking: in a world and not paused
    bool ownWorld();    // the world runs on this PC (single-player / hosting)

    // Game thread (inside a PlayerTick task)
    std::optional<ActorRefs> playerRefs(Actor& player);
    // The stack in the selected hotbar slot, nullptr if the hand is empty
    ItemStack* heldItem(Actor& player, const char* messageIfEmpty);
}
