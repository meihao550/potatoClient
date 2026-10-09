#pragma once
#include "Actor.h"
#include <vector>

namespace ActorList {
    // Every actor the client level knows about (mobs, players, items, ...), written into `out`
    // (cleared first). Reusing the same vector every tick avoids an allocation per tick.
    // Game thread only - call it from onTick.
    void get(Actor& player, std::vector<Actor*>& out);
}
