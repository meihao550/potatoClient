#pragma once
#include "Actor.h"
#include <vector>

namespace ActorList {
    // Every actor the client level knows about (mobs, players, items, ...).
    // Game thread only - call it from onTick.
    std::vector<Actor*> get(Actor& player);
}
